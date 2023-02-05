
import subprocess, sys

tgt = sys.argv[1]

def RUN(cmd):
	subprocess.check_call(
		cmd,
		shell=True
	)

def run_test():
	RUN("clang -o test.exe -Wall -g tests.cpp && test")
def run_bench():
	RUN("clang -o bench.exe -Wall -g -O2 bench.cpp benchutil.cpp -lkernel32 && bench")

locals()["run_" + tgt]()
