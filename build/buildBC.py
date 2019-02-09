from joblib import Parallel, delayed
import os
from colorama import init, Fore, Back, Style
from llvm import runClang, mkdir, runLink, runLLC, run, msvcvarsall

def buildCXXToBC(file, targetpathE, builddebug, nativeSymbols, cppversion, is64, additionalcmd):
    isplainc = file.endswith(".c")
    if file.endswith(".cpp") or file.endswith(".c") or file.endswith(".cc") or file.endswith(".cxx"):
        print(os.path.basename(file))
        runClang([
            file,
            '-emit-llvm',
            '-o', os.path.join(targetpathE, os.path.basename(file) + '.bc'),
            '-c', # Compile only (no link, no native assembly)

            '-m64' if is64 else '-m32',

            '--debug' if nativeSymbols else '',
            '-gcodeview' if nativeSymbols else ('-g' if builddebug else ''), # Debug info (in codeview format - for msvc)

            #'-D_MT', # MSVC: Use this for both MT and MD
            #'-D_DLL', # MSVC: use Dynamic CRT (MD or MDd)
            #'-Xlinker /NODEFAULTLIB',
            '-D_DEBUG' if builddebug else '', # Sets _ITERATOR_DEBUG_LEVEL = 2 instead of 0

            '-Wno-deprecated-declarations', # TODO disable this otherwise
            '-Wno-microsoft-template', # TODO disable this otherwise

            cppversion if not isplainc else '',
            
            ] + additionalcmd + ([
                '-Oz',
                #'-flto=thin', # Activate ThinLTO https://clang.llvm.org/docs/ThinLTO.html
                '-flto',
                #'-fno-lto'

                "-ffunction-sections", # Will reduce binary size https://github.com/android-ndk/ndk/issues/133
                #"-fdata-sections",

                #"-no-canonical-prefixes", # probably not necessary
                "-fno-integrated-as", # disable integrated assembler

                #"-fno-rtti",
                #'-fno-rtti-data',
                #"-fno-exceptions",
                #"-fno-strict-aliasing",

                "-fomit-frame-pointer", # Small functions might become even smaller (disabled by default)
               
                '-fno-stack-protector', 
                #"-fstack-protector-strong", # protect the stack of functions considered as vulnearable
                #"-fno-unwind-tables",
                #"-funwind-tables",

                '-fmerge-all-constants',
                #'-fno-math-errno',
                '-fvisibility=hidden', # linker might strip even more if it is sure this is not a library to link againsts 
            ] if not builddebug else []), False)

def buildBCLib(target, sourcepaths, builddebug, nativeSymbols, onesteplinking, cppversion, buildInParallel, is64, useNativeCompiler, additionalcmd):
    targetpathE = target
    outname = target + '.bc'
    parallel = buildInParallel

    # Don't rebuild, if the library is still there AND none of the files in the sourcepaths has been changed
    dontrebuild = True
    if not os.path.isfile(outname):
        dontrebuild = False
    else:
        parCreation = os.path.getmtime(outname)
        for sourcepath in sourcepaths:
            files = []
            if(os.path.isdir(sourcepath)): # Read everything in that folder
                files = os.listdir(sourcepath)
            elif(os.path.isfile(sourcepath)): # Read a single file
                files = [sourcepath]
                sourcepath = ''
            for f in files:
                if os.path.getmtime(os.path.join(sourcepath, f)) >= parCreation:
                    dontrebuild = False
    if dontrebuild: 
        return False

    print(Fore.LIGHTWHITE_EX + "\nBuilding " + os.path.basename(target) + Fore.RESET)
    mkdir(targetpathE)

    # Get all the files
    paths = []
    for sourcepath in sourcepaths:
        if(os.path.isdir(sourcepath)): # Read everything in that folder
            additionalcmd = additionalcmd + ['-I' + sourcepath]
            for f in os.listdir(sourcepath):
                if f.endswith(".cpp") or f.endswith(".c") or f.endswith(".cc") or f.endswith(".cxx"):
                    paths += [os.path.join(sourcepath,f)]
        else: # Read a single file
            additionalcmd = additionalcmd + ['-I' + os.path.dirname(sourcepath)]
            f = sourcepath
            if f.endswith(".cpp") or f.endswith(".c") or f.endswith(".cc") or f.endswith(".cxx"):
                paths += [f]
      
    if useNativeCompiler:
      
        
        for file in paths:
            run(["cmd", '/C cd ' + msvcvarsall + ' & vcvarsall.bat ' + ('x64' if is64 else 'x86') + ' & cd ' + os.path.dirname(os.path.dirname(os.path.abspath(__file__))) + ' &'
                +'cl ' + " ".join([file] + list(map(lambda x: '/' + x[1:], additionalcmd)) + [
                    # "/GS", # Buffer safety checks
                    "/GL", # Optimize whole programm
                    "/analyze-", # no code analysis
                    "/W0", # Warning level
                    "/Gy", # function level linking
                    "/Zc:wchar_t", 
                    "/Zc:inline",
                    "/fp:precise", # floating point
                    "/DNDEBUG",
                    "/c", # compile without linking
                    "/WX-", # no warnings as errors
                    "/Zc:forScope", 
                    "/Gd", # __cdecl calling convention
                    "/Oy", # omit frame pointer
                    "/Oi", # create system function
                    "/MT",
                    #"/Fa"+targetpathE+"", # assembly list file
                    "/EHsc", # exceptions: c++
                    "/nologo", # suppress showing start information
                    "/Ox", # full optimization
                    "/sdl-", # without additional security features
                    "/Fo"+ os.path.join(targetpathE, os.path.basename(file)) +".ms.obj", # object file
                ])])      
            
        run(["cmd", '/C cd ' + msvcvarsall + ' & vcvarsall.bat ' + ('x64' if is64 else 'x86') + ' & cd ' + os.path.dirname(os.path.dirname(os.path.abspath(__file__))) + ' & cd ' + targetpathE + " & "
                +'lib ' + " ".join(
                    list(map(lambda x: os.path.basename(x) + ".ms.obj", paths)) 
                    + [
                        "/LTCG",
                        "/nologo",
                        "/OUT:../"+os.path.basename(outname) + ".lib",
                        "/MACHINE:" + "X64" if is64 else "X86",
                ])]) 
  

    else:
        if parallel:        
            Parallel(n_jobs=-1)(delayed(buildCXXToBC)
                    (filepath, targetpathE, builddebug, nativeSymbols, cppversion, is64, additionalcmd) 
                    for filepath in paths)
        else:
            for filepath in paths: 
                buildCXXToBC(filepath, targetpathE, builddebug, nativeSymbols, cppversion, is64, additionalcmd)
        
        if not onesteplinking:
            runLink([
                '-o', outname,
            ] + [ os.path.join(targetpathE, os.path.basename(filepath) + ".bc") for filepath in paths])
        
    return outname


def keepLinked_doit(target, bcfiles):
    runLink(['-o', target] + bcfiles)

def keepLinked(target, bcfiles):
    if not os.path.isfile(target):
        keepLinked_doit(target, bcfiles)
    else: 
        parCreation = os.path.getmtime(target)
        changed = False
        for f in bcfiles:
            if os.path.getmtime(f) >= parCreation:
                changed = True
        if changed:
            keepLinked_doit(target, bcfiles)

            
def keepObj(target, source):
    if not os.path.isfile(target) or os.path.getmtime(target) < os.path.getmtime(source):
        runLLC([ 
            '-filetype=obj',
            '-o', target,
            source
        ])