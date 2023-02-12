
import time, os, subprocess, sys

tgt = sys.argv[1]

def RUN(cmd, **kwargs):
	subprocess.check_call(
		cmd,
		shell=True,
		**kwargs
	)

def SAFEDEL(name):
	if os.path.isfile(name):
		rnd = str(round(time.time() * 1000 % 1000))
		dst = name + rnd
		os.rename(name, dst)
		os.remove(dst)

# https://github.com/microsoft/vswhere/wiki/Find-VC
class VCINFO:
	def RUN32(self, *args, **kwargs):
		bk = os.environ.copy()
		os.environ.update(self.env32)
		RUN(*args, **kwargs)
		os.environ.clear()
		os.environ.update(bk)
	def RUN64(self, *args, **kwargs):
		bk = os.environ.copy()
		os.environ.update(self.env64)
		RUN(*args, **kwargs)
		os.environ.clear()
		os.environ.update(bk)

MSVC = None
def GET_MSVC():
	global MSVC
	if MSVC is None:
		pf86 = os.getenv("ProgramFiles(x86)")
		vs_path = subprocess.check_output([
			pf86 + r"\Microsoft Visual Studio\Installer\vswhere.exe",
			"-latest",
			"-products", "*",
			"-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
			"-property", "installationPath"
		]).strip().decode()
		print("vs_path =", vs_path)
		envsrc32 = subprocess.check_output(
			f"cmd /s /c \"\"{vs_path}\\VC\\Auxiliary\\Build\\vcvars32\">NUL && set\"",
			shell=True
		)
		env32 = os.environ.copy()
		for line in envsrc32.decode().splitlines():
			if "=" in line:
				k, v = line.split("=", 1)
				#print(k, "=", v)
				env32[k] = v
		envsrc64 = subprocess.check_output(
			f"cmd /s /c \"\"{vs_path}\\VC\\Auxiliary\\Build\\vcvars64\">NUL && set\"",
			shell=True
		)
		env64 = os.environ.copy()
		for line in envsrc64.decode().splitlines():
			if "=" in line:
				k, v = line.split("=", 1)
				#print(k, "=", v)
				env64[k] = v
		MSVC = VCINFO()
		MSVC.env32 = env32
		MSVC.env64 = env64
	return MSVC

CLANG_WARNINGS = "-Wall -Wextra -Wcast-align -Wcast-qual"
MSVC_WARNINGS = "/W4"
def BUILDTEST(text):
	with open("buildtest.gen.cpp", "w") as f:
		f.write(text)
	print("- Clang 32-bit")
	RUN(f"clang -o buildtest.gen.exe -m32 {CLANG_WARNINGS} buildtest.gen.cpp")
	print("- Clang 64-bit")
	RUN(f"clang -o buildtest.gen.exe -m64 {CLANG_WARNINGS} buildtest.gen.cpp")
	if os.name == "nt":
		msvc = GET_MSVC()
		SAFEDEL("buildtest.gen.exe")
		print("- MSVC 32-bit")
		msvc.RUN32(f"cl /nologo {MSVC_WARNINGS} buildtest.gen.cpp")
		SAFEDEL("buildtest.gen.exe")
		print("- MSVC 64-bit")
		msvc.RUN64(f"cl /nologo {MSVC_WARNINGS} buildtest.gen.cpp")
		SAFEDEL("buildtest.gen.exe")

def fmtsize(n):
	if n < 10240:
		return str(n) + " B"
	n /= 1024
	return "%.2f KB" % n

def BUILDOBJTEST(text):
	with open("buildtest.gen.cpp", "w") as f:
		f.write(text)
	RUN(f"clang -o buildtest.gen.obj -m32 {CLANG_WARNINGS} -c buildtest.gen.cpp")
	print("- Clang 32-bit size = " + fmtsize(os.path.getsize("buildtest.gen.obj")))
	RUN(f"clang -o buildtest.gen.obj -m64 {CLANG_WARNINGS} -c buildtest.gen.cpp")
	print("- Clang 64-bit size = " + fmtsize(os.path.getsize("buildtest.gen.obj")))
	if os.name == "nt":
		msvc = GET_MSVC()
		SAFEDEL("buildtest.gen.obj")
		msvc.RUN32(f"cl /nologo {MSVC_WARNINGS} /c buildtest.gen.cpp")
		print("- MSVC 32-bit size = " + fmtsize(os.path.getsize("buildtest.gen.obj")))
		SAFEDEL("buildtest.gen.obj")
		msvc.RUN64(f"cl /nologo {MSVC_WARNINGS} /c buildtest.gen.cpp")
		print("- MSVC 64-bit size = " + fmtsize(os.path.getsize("buildtest.gen.obj")))
		SAFEDEL("buildtest.gen.obj")

def run_test():
	RUN("clang -o test.exe -Wall -g tests.cpp && test")
def run_benchinternals():
	RUN(
		"clang -o benchinternals.exe -Wall -g -O2"
		" benchinternals.cpp benchutil.cpp -lkernel32 && benchinternals"
	)
def run_benchfiles():
	validate = len(sys.argv) < 3 or sys.argv[2] == "check"
	validate_defs = "" if validate else "-DDATO_VALIDATE_BUFFERS=0 -DDATO_VALIDATE_INPUTS=0"
	for i in range(0, 5):
		print("config =", i)
		RUN(
			"clang -o benchfiles.exe -Wall -g -O2 -fno-exceptions -fno-rtti"
			" benchfiles.cpp benchutil.cpp -DCONFIG=%d %s"
			" -lkernel32 && benchfiles gen-nodes" % (i, validate_defs)
		)

def run_objtest():
	print("=== running object size tests ===")
	print("-- reader / dump --")
	with open("buildtest-reader.cpp", "r") as f:
		suffix = f.read()
		text = '#include "../dato_reader.hpp"\n'
		text += '#include "../dato_dump.hpp"\n#define CANDUMP\n'
		text += suffix
		BUILDOBJTEST(text)
	print("-- writer --")
	with open("buildtest-writer.cpp", "r") as f:
		suffix = f.read()
		incline = '#include "../dato_writer.hpp"'
		text = f"{incline}\n{suffix}\n"
		BUILDOBJTEST(text)

def run_buildtest():
	print("=== running build tests ===")
	print("-- different validation configs (reader/dump) --")
	def_dvb = [
		"",
		"#define DATO_VALIDATE_BUFFERS 0",
		"#define DATO_VALIDATE_BUFFERS 1",
	]
	def_dvi = [
		"",
		"#define DATO_VALIDATE_INPUTS 0",
		"#define DATO_VALIDATE_INPUTS 1",
	]
	inclines = [
		'#include "../dato_reader.hpp"',
		#'#include "../dato_writer.hpp"',
		'#include "../dato_dump.hpp"\n#define CANDUMP',
	]
	with open("buildtest-reader.cpp", "r") as f:
		suffix = f.read()
	for dvb in def_dvb:
		for dvi in def_dvi:
			for incline in inclines:
				print(f"{dvb} / {dvi} / {incline}")
				text = f"{dvb}\n{dvi}\n{incline}\n{suffix}\n"
				BUILDTEST(text)
	print("-- different validation configs (writer) --")
	with open("buildtest-writer.cpp", "r") as f:
		suffix = f.read()
	for dvi in def_dvi:
		incline = '#include "../dato_writer.hpp"'
		print(f"{dvi} / {incline}")
		text = f"{dvi}\n{incline}\n{suffix}\n"
		BUILDTEST(text)

locals()["run_" + tgt]()
