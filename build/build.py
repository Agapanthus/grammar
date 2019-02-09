import os
import sys
from llvm import mkdir, runClang, runLink, fatalError, run, runLLC, llvmbindir, addIgnore, setLLVMBindir, msvcvarsall
from buildBC import keepLinked, buildBCLib, keepObj
from windows import keepNativeExe, keepNativeStaticLib, initWindows

################################################

# Debug build or not
debug = True
nativeSymbols = False

# Optimize the binary
optim = not debug

# Build for 64 bits (TODO: Why does 32 bits not work?)
is64 = True

nativeLibs = []
libraries = dict()
libraryDefs = dict()
libraryVersions = dict()
libraryIncs = dict()

buildInParallel = False

autodiscoversdk = False

projectname = "my_project"

def initNewlib(name):
    libraries[name] = []
    libraryDefs[name] = []
    libraryVersions[name] = []
    libraryIncs[name] = []


with open("config.txt") as f:
    content = f.readlines()
content = [x.strip() for x in content] 
for i in range(len(content)):
    x = content[i]
    if len(x) > 0:
        if x[0] != '#':
            parts = x.split(":")
            if len(parts) != 2:
                for k in range(2,len(parts)):
                    parts[1] += ":"  + parts[k]
                #print("joined: " + parts[1])
                #fatalError("Error in config.txt, line " + str(i+1) + ": Expected exactly one ':'")
                
            parts[1] = parts[1].strip()
            parts[0] = parts[0].strip()

            if parts[0] == 'name':
                projectname = parts[1]
            elif parts[0] == 'configuration':
                if parts[1] == 'debug':
                    debug = True
                elif parts[1] == 'release':
                    debug = False
                elif parts[1] == 'nativedebug':
                    debug = True
                    nativeSymbols = True
                else:
                    fatalError("Error in config.txt, line " + str(i+1) + ": Unknown configuration '" + parts[1] + "'")
            elif parts[0] == 'platform':
                if parts[1] == 'x64':
                    is64 = True
                elif parts[1] == 'x86':
                    is64 = False
                else:
                    fatalError("Error in config.txt, line " + str(i+1) + ": Unknown platform '" + parts[1] + "'")
            elif parts[0] == 'parallel':
                if parts[1] == 'true':
                    buildInParallel = True
                elif parts[1] == 'false':
                    buildInParallel = False
                else:
                    fatalError("Error in config.txt, line " + str(i+1) + ": parallel should be true or false, not '" + parts[1] + "'")
            elif parts[0] == 'include':
                incl = parts[1].split("@")
                if len(incl) != 2:
                    fatalError("Error in config.txt, line " + str(i+1) + ": Expected exactly one '@'")
                else:
                    incl[1] = incl[1].strip()
                    incl[0] = incl[0].strip()
                    if not (incl[1] in libraries):
                        initNewlib(incl[1])
                    libraryIncs[incl[1]] += [incl[0]]
            elif parts[0] == 'llvmbindir':
                setLLVMBindir(parts[1])  # TODO: Doesn't work   
            elif parts[0] == 'ignore':     
                addIgnore(parts[1])  
            elif parts[0] == 'library':
                libpath = parts[1].split("@")
                if len(libpath) == 3:
                    libpath = [x.strip() for x in libpath] 
                    fileList = libpath[2].split(" ")
                    fileList = [x.strip() for x in fileList] 
                    if not (libpath[1] in libraries):
                        initNewlib(libpath[1])
                    for file in fileList:
                        libraries[libpath[1]] += [os.path.join(libpath[0], file)]
                elif len(libpath) != 2:
                    fatalError("Error in config.txt, line " + str(i+1) + ": Expected exactly one '@'")
                else:
                    libpath[1] = libpath[1].strip()
                    libpath[0] = libpath[0].strip()
                    if not (libpath[1] in libraries):
                        initNewlib(libpath[1])
                    libraries[libpath[1]] += [libpath[0]]
            elif parts[0] == 'define':
                definition = parts[1].split("@")
                if len(definition) != 2:
                    fatalError("Error in config.txt, line " + str(i+1) + ": Expected exactly one '@'")
                else:
                    definition[1] = definition[1].strip()
                    definition[0] = definition[0].strip()
                    if not (definition[1] in libraries):
                        initNewlib(definition[1])
                    libraryDefs[definition[1]] += [definition[0]]
            elif parts[0] == 'version':
                version = parts[1].split("@")
                if len(version) != 2:
                    fatalError("Error in config.txt, line " + str(i+1) + ": Expected exactly one '@'")
                else:
                    version[1] = version[1].strip()
                    version[0] = version[0].strip()
                    if not (version[1] in libraries):
                        initNewlib(version[1])
                    libraryVersions[version[1]] += [version[0]]
            elif parts[0] == 'nativelibrary':
                nativeLibs += [parts[1]]
            elif parts[0] == 'useNativeSDK':
                if parts[1] == 'false':
                    autodiscoversdk = False
                elif parts[1] == 'true':
                    autodiscoversdk = True
                else:
                    fatalError("Error in config.txt, line " + str(i+1) + ": Unknown boolean '" + parts[1] + "'")
            else:
                fatalError("Error in config.txt, line " + str(i+1) + ": Unknown property '" + parts[0] + "'")


if len(sys.argv) > 1:
    if sys.argv[1] == 'Debugx64n':
        debug = True
        nativeSymbols = True
        is64 = True
    else:
        fatalError("Invalid argv: " + str(sys.argv))


initWindows(autodiscoversdk)
if not projectname in libraries.keys():
    fatalError("Project '" + projectname + "' not found." )

################################################

platform = ("x64" if is64 else "x86")
targetArch = "x86-64" if is64 else "i386"
vendor = 'pc'
sys = 'win32'
abi = 'eabi'
# <arch><sub>-<vendor>-<sys>-<abi>, see https://clang.llvm.org/docs/CrossCompilation.html
#targetTriple =  targetArch + "-" + vendor + '-' + sys + '-' + abi 

# Don't do LTO
fastLinking = debug

Configuration = ("Debug" if debug else "Release") + platform + ("n" if nativeSymbols else "")


binPath = './bin/' + Configuration + "/"
bitPath = binPath + "bitcode/"
mkdir(binPath)
mkdir(bitPath)

################################################

onesteplinking = False # Won't work on windows. Cmd too long.
useNativeCompiler = False # Will use cl.exe, lib.exe and link.exe

for library in libraries:
    buildBCLib(bitPath + library, libraries[library], debug, nativeSymbols, onesteplinking, libraryVersions[library][0], buildInParallel, is64, useNativeCompiler,
        list(map(lambda x: '-I'+x, libraries[library] + libraryIncs[library])) 
        + list(map(lambda x: '-D'+x, libraryDefs[library]))
    )
    if library != projectname and not onesteplinking and not useNativeCompiler:
        keepObj(bitPath + library + ".o", bitPath + library + ".bc")
        #if fastLinking:
        #    keepNativeStaticLib(bitPath + library + ".o", bitPath + library + ".lib", platform)


if fastLinking:
    keepNativeExe([bitPath + projectname + '.bc'], binPath + projectname, platform, debug, nativeSymbols, is64, 
        list(map(lambda x: bitPath + x + '.o', # '.lib', 
        filter(lambda x: x != projectname, list(libraries.keys()))))
         + nativeLibs )
else:  

    if onesteplinking:
        bclist = []
        for fol in os.listdir(bitPath):
            bfol = os.path.join(bitPath, fol)
            if(os.path.isdir(bfol)):
                for f in os.listdir(bfol):
                    bclist = bclist + [os.path.join(bfol, f)]


        keepNativeExe(bclist,
            binPath + projectname, platform, debug, nativeSymbols, is64, [] + nativeLibs) 
    elif useNativeCompiler:
        mainProjectBitcode = os.path.join(bitPath, projectname)+"/"
        run(["cmd", '/C cd ' + msvcvarsall + ' & vcvarsall.bat ' + ('x64' if is64 else 'x86') + ' & cd ' + os.path.dirname(os.path.dirname(os.path.abspath(__file__))) + ' &'
            +'link ' + " ".join(
                list(map(lambda x: bitPath + x + '.bc.lib', filter(lambda x: x != projectname, list(libraries.keys()))))
                + list(map(lambda x: mainProjectBitcode + x, os.listdir(mainProjectBitcode) ))
                + [
                    #"/IMPLIB:" + bitPath + projectname + ".bc.lib",
                    "/INCREMENTAL:NO",
                    "/SUBSYSTEM:CONSOLE",
                    "/nologo",
                    "/LTCG",
                    "/OUT:"+ binPath + projectname + ".ms.exe",
                    "/MACHINE:" + "X64" if is64 else "X86",
                    "/MANIFEST",
                    "/NXCOMPAT",
                    "/DYNAMICBASE",
                    "kernel32.lib",
                    "user32.lib",
                    "gdi32.lib",
                    "winspool.lib",
                    "shell32.lib",
                    "ole32.lib",
                    "oleaut32.lib",
                    "uuid.lib" 
            ])]) 
  
    else:
        keepNativeExe(
            [bitPath + projectname + '.bc'] 
            + list(map(lambda x: bitPath + x + '.bc', # TODO: Why can't we optimize it even more?
            filter(lambda x: x != projectname, list(libraries.keys())))),
            binPath + projectname, platform, debug, nativeSymbols, is64, [] + nativeLibs) 


if not nativeSymbols:
    run([binPath + projectname], True, True)

