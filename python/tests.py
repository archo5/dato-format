#!/usr/bin/python3

import struct
import sys
from time import time

from datofmt import (Builder, LinearWriter, encode, BufferReader, decode, Validator)

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

def CHKVALID(ret):
	if ret[0] is not True:
		print("ERROR on line %d" % sys._getframe(1).f_lineno)
		print("  expected:", (True, None))
		print("  got:     ", ret)

def FVALID(ref):
	ret = Validator().validate(ref)
	if ret[0] is not True:
		print("ERROR on line %d" % sys._getframe(1).f_lineno)
		print("  expected:", (True, None))
		print("  got:     ", ret)
	else:
		print(decode(ref))
	return ref

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
	HDR = b"DATO" + B0 + B(1)
	HDRALIGN = HDR + b"\0\0"
	CHKEQ(encode({}), FVALID(HDRALIGN+U(12)+U(0)))
	CHKEQ(encode({"a":None}), FVALID(HDRALIGN+U(20)+U(1)+b"a\0\0\0"+U(1)+U(12)+U(0)+B0))
	CHKEQ(encode({"b":True}), FVALID(HDRALIGN+U(20)+U(1)+b"b\0\0\0"+U(1)+U(12)+U(1)+B(1)))
	CHKEQ(encode({"c":False}), FVALID(HDRALIGN+U(20)+U(1)+b"c\0\0\0"+U(1)+U(12)+U(0)+B(1)))
	def _():
		b = LinearWriter()
		b.write_int32("abc", -23456)
		ref = HDRALIGN+U(20)+U(3)+b"abc"+B0+U(1)+U(12)+U(2**32-23456)+B(2)
		CHKEQ(b.get_encoded(), FVALID(ref))
	_()
	def _():
		b = LinearWriter()
		b.write_uint32("bcd", 12345)
		ref = HDRALIGN+U(20)+U(3)+b"bcd"+B0+U(1)+U(12)+U(12345)+B(3)
		CHKEQ(b.get_encoded(), FVALID(ref))
	_()
	def _():
		b = LinearWriter()
		b.write_float32("cde", 1.25)
		ref = HDRALIGN+U(20)+U(3)+b"cde"+B0+U(1)+U(12)+F(1.25)+B(4)
		CHKEQ(b.get_encoded(), FVALID(ref))
	_()
	def _():
		b = LinearWriter()
		b.write_int64("def", -12345654321)
		ref = HDRALIGN+U(32)+U(0)+UU(2**64-12345654321)+U(3)+b"def"+B0+U(1)+U(24)+U(16)+B(5)
		CHKEQ(b.get_encoded(), FVALID(ref))
	_()
	def _():
		b = LinearWriter()
		b.write_uint64("efg", 23456765432)
		ref = HDRALIGN+U(32)+U(0)+UU(23456765432)+U(3)+b"efg"+B0+U(1)+U(24)+U(16)+B(6)
		CHKEQ(b.get_encoded(), FVALID(ref))
	_()
	def _():
		b = LinearWriter()
		b.write_float64("fgh", 100000000000000000000.0)
		ref = HDRALIGN+U(32)+U(0)+UU(0x4415AF1D78B58C40)+U(3)+b"fgh"+B0+U(1)+U(24)+U(16)+B(7)
		CHKEQ(b.get_encoded(), FVALID(ref))
	_()
	def _():
		ref = HDRALIGN+U(24)+U(3)+b"ghi\0"+U(0)+U(1)+U(12)+U(20)+B(8)
		CHKEQ(encode({"ghi":[]}), FVALID(ref))
	_()
	def _():
		ref = HDRALIGN+U(24)+U(3)+b"hij\0"+U(0)+U(1)+U(12)+U(20)+B(9)
		CHKEQ(encode({"hij":{}}), FVALID(ref))
	_()
	def _():
		b = LinearWriter()
		b.write_string_utf8("ijk", "!@#")
		ref = HDRALIGN+U(28)+U(3)+b"!@#\0"+U(3)+b"ijk\0"+U(1)+U(20)+U(12)+B(10)
		CHKEQ(b.get_encoded(), FVALID(ref))
	_()
_()
