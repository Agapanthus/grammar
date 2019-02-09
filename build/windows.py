from llvm import mkdir, runClang, runLink, fatalError, run, runLLC, llvmbindir, printErrorWarn
import os

winpath = os.environ['WINDIR'] + "\\System32\\"

windowssdk = ""
msvcsdk = ""
libExe = ""

def initWindows(autodiscoversdk):
    global windowssdk
    global msvcsdk
    global libExe
    
    if autodiscoversdk:

        ########### Windows SDK Detection
        windowskits = os.environ['programfiles(x86)'] + "\\Windows Kits\\"
        # Choose a version of the Windows SDK which has the "um"-subdirectory
        windowskitsversions = [
            os.path.join(windowskits+ "10\\Lib\\", o) 
                for o
                in os.listdir(windowskits+ "10\\Lib\\") 
                if os.path.isfile(os.path.join(os.path.join(windowskits+ "10\\Lib\\",o), "um\\x86\\User32.Lib")) 
                or os.path.isfile(os.path.join(os.path.join(windowskits+ "10\\Lib\\",o), "um\\x86\\User32.lib"))
            ]
        if len(windowskitsversions) <= 0: fatalError("ERROR: Windows SDK not found") 
        windowssdk = windowskitsversions[0]

        ############# Visual Studio SDK Detection
        msvclibbase = os.environ['programfiles(x86)'] + "\\Microsoft Visual Studio\\2017\\Community\\VC\\Tools\\MSVC\\"
        # Choose a version of Visual Studio
        msvclibs = [
            os.path.join(msvclibbase, o) 
                for o
                in os.listdir(msvclibbase) 
                if os.path.isfile(os.path.join(os.path.join(msvclibbase,o), "lib\\x86\\libcmt.lib"))
            ]
        if len(msvclibs) <= 0: fatalError("ERROR: MSVC SDK not found") 
        msvcsdk = msvclibs[0]

        #print("Discovered WinSDK-Path:")
        #print(windowssdk)
        #print(msvcsdk)

    else:
        windowssdk = "../winsdk/"
        msvcsdk = "../winsdk/"
    libExe = os.path.join(msvcsdk, "bin/Hostx86/x86/lib.exe" if autodiscoversdk else 'lib.exe')


###################################

def keepNativeStaticLib(objectFile, target, platform):
    if not os.path.isfile(target) or os.path.getmtime(target) < os.path.getmtime(objectFile):
        makeNativeStaticLib(objectFile, target, platform)

def makeNativeStaticLib(objectFile, target, platform):
    run([
            libExe,
            objectFile,
            '/OUT:' + target,
        ])

def keepNativeExe(objectfiles, target, platform, debug, nativeSymbols, is64, additionalLibs = []):
    # Don't rebuild, if the exe is still there AND none of the objectfiles have been changed
    dontrebuild = True
    texe = target+ ".exe"
    if nativeSymbols:
        texe = target + '/../debugee.exe'
    if not os.path.isfile(texe):
        dontrebuild = False
    else:
        parCreation = os.path.getmtime(texe)
        for file in objectfiles:
            if os.path.getmtime(file) >= parCreation:
                dontrebuild = False
    if dontrebuild: 
        return False

    makeNativeExe(objectfiles, target, platform, debug, nativeSymbols, is64, additionalLibs)


def makeNativeExe(objectfiles, target, platform, debug, nativeSymbols, is64, additionalLibs = []):

    if nativeSymbols:
        if not debug: 
            printErrorWarn("Error: native symbols only in debug mode")
        
        
        runClang(objectfiles + [
            "-o" + target + '/../debugee.exe',
            "--debug",
            '-m64' if is64 else '-m32',
            '-gcodeview',
            #'-Xlinker /NODEFAULTLIB',
            
            os.path.join(msvcsdk,'lib\\' + platform) + '\\libcmtd.lib', 

            #os.path.join(msvcsdk,'lib\\' + platform) + '\\libcpmtd.lib',
            os.path.join(msvcsdk,'lib\\' + platform) + '\\libvcruntimed.lib',
            os.path.join(msvcsdk,'lib\\' + platform) + '\\oldnames.lib',
            os.path.join(msvcsdk,'lib\\' + platform) + '\\legacy_stdio_definitions.lib',
            os.path.join(msvcsdk,'lib\\' + platform) + '\\legacy_stdio_wide_specifiers.lib',
        ] + additionalLibs)

    else:

        #runLink([
        #    '-o', target + ".exe.bc",
        #] + objectfiles) #additionalLibs
        
        #run([llvmbindir + 'opt', 
        #    '-Oz',
        #    '-o', target + ".exe.opt.bc",
        #    target + ".exe.bc",
        #])

        """
        runClang([
            "-o" + target + '.exe',
            '-m64' if is64 else '-m32',
            #'-O3' if not debug else '',
            target + ".exe.opt.bc",
           
            os.path.join(msvcsdk,'lib\\' + platform) + '\\libcmtd.lib', 
            os.path.join(msvcsdk,'lib\\' + platform) + '\\libvcruntimed.lib',
            os.path.join(msvcsdk,'lib\\' + platform) + '\\oldnames.lib',
            os.path.join(msvcsdk,'lib\\' + platform) + '\\legacy_stdio_definitions.lib',
            os.path.join(msvcsdk,'lib\\' + platform) + '\\legacy_stdio_wide_specifiers.lib',
        ] + additionalLibs)
        """
        """
        runClang(objectfiles + [
            "-o" + target + '.exe',
            '-m64' if is64 else '-m32',
            #'-Oz' if not debug else '',
           
            os.path.join(msvcsdk,'lib\\' + platform) + '\\libcmtd.lib', 
            os.path.join(msvcsdk,'lib\\' + platform) + '\\libvcruntimed.lib',
            os.path.join(msvcsdk,'lib\\' + platform) + '\\oldnames.lib',
            os.path.join(msvcsdk,'lib\\' + platform) + '\\legacy_stdio_definitions.lib',
            os.path.join(msvcsdk,'lib\\' + platform) + '\\legacy_stdio_wide_specifiers.lib',
        ] + additionalLibs)
        """

        # lld allows multithreading. TODO: How...?    -Wl,--threads -Wl,--thread-count,xxx
        
        run([llvmbindir + 'lld-link'] + objectfiles + [
            '/out:' + target + '.exe',
            '/machine:' + platform,
            '/nodefaultlib',
            '/subsystem:console',
            '/opt:lldltojobs=8', # Multithreaded ThinLTO
            '/libpath:'+os.path.join(msvcsdk,'lib\\' + platform)+'',
            '/libpath:'+os.path.join(windowssdk,'ucrt\\' + platform)+'',
            '/libpath:'+os.path.join(windowssdk,'um\\' + platform)+'',

            '/libpath:./third_party/llvm/cmake_build/' + ("Debug" if debug else "Release") + '/lib',

            #'/errorlimit:0', # Show all errors
        
            ] + additionalLibs + [

            
            # About CRT on Windows https://msdn.microsoft.com/en-us/library/abx4dbyh.aspx
            # Also useful: Read linker Directives from lib's and o's using the VS Developer Command Prompt:
            # ``` dumpbin /DIRECTIVES yourlibrary.o ``` 
            # See also https://github.com/Leandros/ClangOnWindows
            'libcpmtd.lib' if debug else 'libcpmt.lib',


            'libucrtd.lib' if debug else 'libucrt.lib', 
            'libvcruntimed.lib' if debug else 'libvcruntime.lib',
            'libcmtd.lib' if debug else 'libcmt.lib', 
            'shell32.lib', # used for SHFileOperation
            'ole32.lib',  # used for CoTaskMemFree
            
            #'uuid.lib',
            #'oleaut32.lib',
            #'gdi32.lib',
            #'winspool.lib',
            #'comdlg32.lib',
            'advapi32.lib', # used for CryptGenRandom
            #'odbc32.lib',
            #'odbccp32.lib',
            #'psapi.lib',
            #'delayimp.lib',
            'concrtd.lib' if debug else 'concrt.lib', # used for Concurrency

            # Necessary for compatibility (Really! They are.)
            'legacy_stdio_definitions.lib', 'oldnames.lib', 'legacy_stdio_wide_specifiers.lib',

            'kernel32.lib', 'User32.lib',
        
        ])
        


