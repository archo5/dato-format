#!/usr/bin/python3

import struct
import sys
from time import time

import datofmt

def PERF(fn, maxtime=0.1, maxiter=100000):
	try:
		t0 = time()
		it = 0
		while it < maxiter and (it == 0 or time() - t0 < maxtime):
			fn()
			it += 1
		t1 = time()
		ems = (t1 - t0) * 1000
		eus = ems * 1000
		print("%s -- n=%d, t=%.2fms, speed=%.2fus/iter" % (str(fn.__name__), it, ems, eus / it))
	except Exception as e:
		print(e)

def CHKEQ(val, ref):
	if val != ref:
		print("ERROR on line %d" % sys._getframe(1).f_lineno)
		print("  expected:", ref)
		print("  got:     ", val)

B0 = b"\x00"
def B(val): return val.to_bytes(1, "little")
def U(val): return val.to_bytes(4, "little")
def UU(val): return val.to_bytes(8, "little")
def F(val): return struct.pack("<f", val)


print("-- overhead --")

def _():
	def overhead(): pass
	PERF(overhead)
_()


print("-- built-ins --")

def _():
	bytes = b"\x01\x02\x03\x04"
	S = struct.Struct("<I")

	def BI_unpack_32_manual():
		return bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24)
	assert(BI_unpack_32_manual() == 0x04030201)
	PERF(BI_unpack_32_manual)

	def BI_unpack_32_structfull():
		return struct.unpack("<I", bytes)[0]
	assert(BI_unpack_32_structfull() == 0x04030201)
	PERF(BI_unpack_32_structfull)

	def BI_unpack_32_structfast():
		return S.unpack(bytes)[0]
	assert(BI_unpack_32_structfast() == 0x04030201)
	PERF(BI_unpack_32_structfast)

	def BI_unpack_32_intfrombytes():
		return int.from_bytes(bytes, "little")
	assert(BI_unpack_32_intfrombytes() == 0x04030201)
	PERF(BI_unpack_32_intfrombytes)
_()


print("-- basic encoding --")

def _():
	HDR = b"DATO" + B0
	HDRALIGN = HDR + b"\0\0\0"
	CHKEQ(datofmt.encode({}), HDRALIGN+U(12)+U(0))
	CHKEQ(datofmt.encode({"a":None}), HDRALIGN+U(20)+U(1)+b"a\0\0\0"+U(1)+U(12)+U(0)+B0)
	CHKEQ(datofmt.encode({"b":True}), HDRALIGN+U(20)+U(1)+b"b\0\0\0"+U(1)+U(12)+U(1)+B(1))
	CHKEQ(datofmt.encode({"c":False}), HDRALIGN+U(20)+U(1)+b"c\0\0\0"+U(1)+U(12)+U(0)+B(1))
	def _():
		b = datofmt.LinearWriter()
		b.write_int32("abc", -23456)
		CHKEQ(b.get_encoded(), HDRALIGN+U(20)+U(3)+b"abc"+B0+U(1)+U(12)+U(2**32-23456)+B(2))
	_()
	def _():
		b = datofmt.LinearWriter()
		b.write_uint32("bcd", 12345)
		CHKEQ(b.get_encoded(), HDRALIGN+U(20)+U(3)+b"bcd"+B0+U(1)+U(12)+U(12345)+B(3))
	_()
	def _():
		b = datofmt.LinearWriter()
		b.write_float32("cde", 1.25)
		CHKEQ(b.get_encoded(), HDRALIGN+U(20)+U(3)+b"cde"+B0+U(1)+U(12)+F(1.25)+B(4))
	_()
	def _():
		b = datofmt.LinearWriter()
		b.write_int64("def", -12345654321)
		CHKEQ(b.get_encoded(), HDRALIGN+U(32)+U(0)+UU(2**64-12345654321)+U(3)+b"def"+B0+U(1)+U(24)+U(16)+B(5))
	_()
	def _():
		b = datofmt.LinearWriter()
		b.write_uint64("efg", 23456765432)
		CHKEQ(b.get_encoded(), HDRALIGN+U(32)+U(0)+UU(23456765432)+U(3)+b"efg"+B0+U(1)+U(24)+U(16)+B(6))
	_()
_()
