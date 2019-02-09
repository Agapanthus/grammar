from colorama import init, Fore, Back, Style
import subprocess
init()
import os
import sys


from threading import Thread
import queue as queue


msvcvarsall = "C:\\Program Files (x86)\\Microsoft Visual Studio 14.0\\VC\\"

llvmbindir = ''
ignoreLines = []

def setLLVMBindir(llvmbd):
    global llvmbindir
    llvmbindir = llvmbd

def addIgnore(ign):
    global ignoreLines
    ignoreLines = ignoreLines + [ign]

def mkdir(directory):
    if not os.path.exists(directory):
        os.makedirs(directory)

def printErrorWarn(str):
    if(not str.isspace()):
        for s in str.splitlines():
            foundIg = False
            for ig in ignoreLines:
                if s.find(ig) >= 0:
                    foundIg = True
                    break
            if foundIg: continue
            if s.find("error") >= 0 or s.find("Error") >= 0 or s.find("error:") >= 0:
                sys.stdout.write(Fore.RED + s + Fore.RESET + "\n")
            elif s.find("warn") >= 0:
                sys.stdout.write(Fore.YELLOW + s + Fore.RESET + "\n")
            elif s.find("note") >= 0:
                sys.stdout.write(Fore.BLUE + s + Fore.RESET + "\n")
            else:
                sys.stdout.write(Fore.CYAN + s + Fore.RESET + "\n")
    else: sys.stdout.write("\n")


def reader(pipe, queue):
    try:
        with pipe:
            for line in iter(pipe.readline, b''):
                queue.put((pipe, line))
    finally:
        queue.put(None)
        
def run(arr, printCmd = True, direct = False):   
    if printCmd:
        sys.stdout.write(Fore.RESET + '  '.join(arr) + "\n") 

    cumu = ""
    process = subprocess.Popen(arr, stdout=subprocess.PIPE, stderr=subprocess.PIPE, bufsize=1)
    q = queue.Queue()
    Thread(target=reader, args=[process.stdout, q]).start()
    Thread(target=reader, args=[process.stderr, q]).start()
    for _ in range(2):
        for source, line in iter(q.get, None):
            ll = "".join(map(chr, line)) #line.decode('utf-8')
            cumu += ll
            #print (source, line)
            printErrorWarn(ll)

    if(cumu.find("error:") >= 0):
        printErrorWarn("Error: Build failed.")
        exit(-1)


def runClang(arr, printCmd = True):
    run([llvmbindir + 'clang'] + arr, printCmd)

def runLink(arr, printCmd = True):
    run([llvmbindir + 'llvm-link'] + arr, printCmd)

def runLLC(arr, printCmd = True):
    run([llvmbindir + 'llc'] + arr, printCmd)

def fatalError(msg):
    printErrorWarn('Error: ' + Fore.RED + msg + Fore.RESET)
    exit(-1)