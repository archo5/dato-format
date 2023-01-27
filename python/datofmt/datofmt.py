
import struct


TYPE_Null = 0
# embedded value types
TYPE_Bool = 1
TYPE_S32 = 2
TYPE_U32 = 3
TYPE_F32 = 4
# external value reference types
# - one value
TYPE_S64 = 5
TYPE_U64 = 6
TYPE_F64 = 7
# - generic containers
TYPE_Array = 8
TYPE_Object = 9
# - raw arrays (identified by purpose)
# (strings contain an extra 0-termination value not included in their size)
TYPE_String8 = 10 # ASCII/UTF-8 or other single-byte encoding
TYPE_String16 = 11 # likely to be UTF-16
TYPE_String32 = 12 # likely to be UTF-32
TYPE_ByteArray = 13
# - typed arrays (identified by type)
TYPE_TypedArrayS8 = 14
TYPE_TypedArrayU8 = 15
TYPE_TypedArrayS16 = 16
TYPE_TypedArrayU16 = 17
TYPE_TypedArrayS32 = 18
TYPE_TypedArrayU32 = 19
TYPE_TypedArrayS64 = 20
TYPE_TypedArrayU64 = 21
TYPE_TypedArrayF32 = 22
TYPE_TypedArrayF64 = 23


PACK_S8 = struct.Struct("<b")
PACK_U8 = struct.Struct("<B")
PACK_S16 = struct.Struct("<h")
PACK_U16 = struct.Struct("<H")
PACK_S32 = struct.Struct("<i")
PACK_U32 = struct.Struct("<I")
PACK_S64 = struct.Struct("<q")
PACK_U64 = struct.Struct("<Q")
PACK_F32 = struct.Struct("<f")
PACK_F64 = struct.Struct("<d")


class EncodingException(Exception):
	pass


def roundup(x, n):
	return (x + n - 1) // n * n


class EncodingU8:
	def write(W, intval, value_alignment):
		if intval < 0 or intval > 255:
			raise EncodingException("expected length between 0 and 255, got %s" % intval)
		pos = len(W.data)
		if value_alignment is not None:
			pos = roundup(pos + 1, value_alignment) - 1
			while len(W.data) < pos:
				W.data.append(0)
		W.data.append(intval)
		return pos
	def parse(R, pos):
		return int(R.data[pos]), pos + 1


class EncodingU32:
	def write(W, intval, value_alignment):
		if intval < 0 or intval > 0xffffffff:
			raise EncodingException("expected length between 0 and 2^32-1, got %s" % intval)
		pos = len(W.data)
		if value_alignment is not None:
			if value_alignment < 4:
				value_alignment = 4
			pos = roundup(pos, value_alignment)
			while len(W.data) < pos:
				W.data.append(0)
		W.data.extend(intval.to_bytes(4, "little"))
		return pos
	def parse(R, pos):
		return int.from_bytes(R.data[pos:pos+4], "little"), pos + 4


class EncodingU8X32:
	def write(W, intval, value_alignment):
		if intval < 0 or intval > 0xffffffff:
			raise EncodingException("expected length between 0 and 2^32-1, got %s" % intval)
		pos = len(W.data)
		if intval < 255:
			if value_alignment is not None:
				pos = roundup(pos + 1, value_alignment) - 1
				while len(W.data) < pos:
					W.data.append(0)
			W.data.append(intval)
		else:
			if value_alignment is not None:
				if value_alignment < 4:
					value_alignment = 4
				pos = roundup(pos + 5, value_alignment) - 5
				while len(W.data) < pos:
					W.data.append(0)
			W.data.append(255)
			W.data.extend(intval.to_bytes(4, "little"))
		return pos
	def parse(R, pos):
		v = int(R.data[pos])
		pos += 1
		if v != 255:
			return v, pos
		else:
			return int.from_bytes(R.data[pos:pos+4], "little"), pos + 4


"""Optimizes for reading speed at the cost of size while remaining compatible with all data"""
class Config0:
	identifier = 0
	aligned = True
	key_length_encoding = EncodingU32
	object_size_encoding = EncodingU32
	array_length_encoding = EncodingU32
	value_length_encoding = EncodingU32


"""Optimizes for size at the cost of reading speed while remaining compatible with all data"""
class Config1:
	identifier = 1
	aligned = False
	key_length_encoding = EncodingU8X32
	object_size_encoding = EncodingU8X32
	array_length_encoding = EncodingU8X32
	value_length_encoding = EncodingU8X32


"""Optimizes for reading speed first, then size, while breaking compatibility with large objects and keys"""
class Config2:
	identifier = 2
	aligned = True
	key_length_encoding = EncodingU8
	object_size_encoding = EncodingU8
	array_length_encoding = EncodingU32
	value_length_encoding = EncodingU32


"""Optimizes for size first, then reading speed, while breaking compatibility with large objects and keys"""
class Config3:
	identifier = 3
	aligned = False
	key_length_encoding = EncodingU8
	object_size_encoding = EncodingU8
	array_length_encoding = EncodingU8X32
	value_length_encoding = EncodingU8X32


class Value:
	def __init__(self, vtype, val):
		self.vtype = vtype
		self.val = val
	def with_key(self, key):
		return Entry(self.vtype, key, self.val)


class Entry:
	def __init__(self, vtype, key, val=0):
		self.vtype = vtype
		self.key = key
		self.val = val


class StagingObject:
	def __init__(self, has_keys):
		self.entries = []
		self.has_keys = has_keys


class Builder:
	def __init__(
		self,
		*,
		skip_duplicate_keys=True,
		prefix=b"DATO",
		config=Config0
	):
		# settings
		if config.identifier >= 4 and config.identifier <= 127:
			raise EncodingException("reserved identifier (4-127) used: %d", config.identifier)
		self.skip_duplicate_keys = skip_duplicate_keys
		self.aligned = config.aligned
		self._append_key_length = config.key_length_encoding.write
		self._append_object_size = config.object_size_encoding.write
		self._append_array_length = config.array_length_encoding.write
		self._append_value_length = config.value_length_encoding.write
		# state
		self.data = bytearray(prefix)
		self.written_keys = {}
		# header
		self.data.append(config.identifier)
		# - reserve space for the root pointer
		if self.aligned:
			pos = roundup(len(self.data), 4)
			while len(self.data) < pos:
				self.data.append(0)
		self.root_pos_off = len(self.data)
		self.data.extend(b"\0\0\0\0")

	def _align8(self):
		pos = len(self.data)
		if self.aligned:
			pos = roundup(pos, 8)
			while len(self.data) < pos:
				self.data.append(0)
		return pos

	def finish(self, root):
		assert(root.vtype == TYPE_Object)
		b = root.val.to_bytes(4, "little")
		o = self.root_pos_off
		self.data[o + 0] = b[0]
		self.data[o + 1] = b[1]
		self.data[o + 2] = b[2]
		self.data[o + 3] = b[3]

	def get_encoded(self):
		return self.data

	def append_key(self, key):
		"""returns the position of the key"""
		key = key.encode("utf-8")

		if self.skip_duplicate_keys:
			wk = self.written_keys.get(key)
			if wk is not None:
				return wk

		pos = self._append_key_length(self, len(key), 1 if self.aligned else None)
		self.data.extend(key)
		self.data.append(0)

		if self.skip_duplicate_keys:
			self.written_keys[key] = pos

		return pos

	def append_null(self):
		return Value(TYPE_Null, 0)

	def append_bool(self, key, val):
		return Value(TYPE_Bool, 1 if val else 0)
	def append_int32(self, val):
		if val < -0x80000000 or val > 0x7fffffff:
			raise EncodingException("value is out of int32 range")
		if val < 0:
			val += 0x100000000
		return Value(TYPE_S32, val)
	def append_uint32(self, val):
		if val < 0 or val > 0xffffffff:
			raise EncodingException("value is out of uint32 range")
		return Value(TYPE_U32, val)
	def append_float32(self, val):
		return Value(TYPE_F32, int.from_bytes(PACK_F32.pack(val), "little"))

	def append_int64(self, val):
		if val < -0x8000000000000000 or val > 0x7fffffffffffffff:
			raise EncodingException("value is out of int64 range")
		pos = self._align8()
		self.data.extend(int(val).to_bytes(8, "little", signed=True))
		return Value(TYPE_S64, pos)
	def append_uint64(self, val):
		if val < 0 or val > 0xffffffffffffffff:
			raise EncodingException("value is out of uint64 range")
		pos = self._align8()
		self.data.extend(int(val).to_bytes(8, "little"))
		return Value(TYPE_U64, pos)
	def write_float64(self, val):
		pos = self._align8()
		self.data.extend(PACK_F64.pack(val))
		return Value(TYPE_F64, pos)

	def append_string(self, val):
		val = str(val).encode("utf-8")
		pos = self._append_value_length(self, len(val), 1 if self.aligned else None)
		self.data.extend(val)
		return Value(TYPE_String8, pos)

	def append_bytes(self, val):
		pos = self._append_value_length(self, len(val), 1 if self.aligned else None)
		self.data.extend(val)
		return Value(TYPE_ByteArray, pos)

	def _append_typed_array(self, val, vtype, enc, size):
		pos = self._append_value_length(self, len(val), size if self.aligned else None)
		for v in val:
			self.data.extend(enc.pack(v))
	def append_typed_array_int8(self, val):
		self._append_typed_array(val, TYPE_TypedArrayS8, PACK_S8, 1)
	def append_typed_array_uint8(self, val):
		self._append_typed_array(val, TYPE_TypedArrayU8, PACK_U8, 1)
	def append_typed_array_int16(self, val):
		self._append_typed_array(val, TYPE_TypedArrayS16, PACK_S16, 2)
	def append_typed_array_uint16(self, val):
		self._append_typed_array(val, TYPE_TypedArrayU16, PACK_U16, 2)
	def append_typed_array_int32(self, val):
		self._append_typed_array(val, TYPE_TypedArrayS32, PACK_S32, 4)
	def append_typed_array_uint32(self, val):
		self._append_typed_array(val, TYPE_TypedArrayU32, PACK_U32, 4)
	def append_typed_array_int64(self, val):
		self._append_typed_array(val, TYPE_TypedArrayS64, PACK_S64, 8)
	def append_typed_array_uint64(self, val):
		self._append_typed_array(val, TYPE_TypedArrayU64, PACK_U64, 8)
	def append_typed_array_float32(self, val):
		self._append_typed_array(val, TYPE_TypedArrayF32, PACK_F32, 4)
	def append_typed_array_float64(self, val):
		self._append_typed_array(val, TYPE_TypedArrayF64, PACK_F64, 8)

	def append_object(self, entries):
		pos = self._append_object_size(self, len(entries), 4 if self.aligned else None)
		for e in entries:
			self.data.extend(e.key.to_bytes(4, "little"))
		for e in entries:
			self.data.extend(e.val.to_bytes(4, "little"))
		for e in entries:
			self.data.append(e.vtype)
		return pos

	def append_object_from_dict(self, objdict):
		entries = [v.with_key(self.append_key(k)) for k, v in objdict.items()]
		return self.append_object(entries)

	def append_array(self, elements):
		pos = self._append_array_length(self, len(elements), 4 if self.aligned else None)
		for e in elements:
			self.data.extend(e.val.to_bytes(4, "little"))
		for e in elements:
			self.data.append(e.vtype)
		return pos


class LinearWriter:
	def __init__(
		self,
		*,
		skip_duplicate_keys=True,
		prefix=b"DATO",
		config=Config0
	):
		# settings
		if config.identifier >= 4 and config.identifier <= 127:
			raise EncodingException("reserved identifier (4-127) used: %d", config.identifier)
		self.skip_duplicate_keys = skip_duplicate_keys
		self.aligned = config.aligned
		self._append_key_length = config.key_length_encoding.write
		self._append_object_size = config.object_size_encoding.write
		self._append_array_length = config.array_length_encoding.write
		self._append_value_length = config.value_length_encoding.write
		# state
		self.data = bytearray(prefix)
		self.stack = [StagingObject(True)]
		self.written_keys = {}
		# header
		self.data.append(config.identifier)
		# - reserve space for the root pointer
		if self.aligned:
			pos = roundup(len(self.data), 4)
			while len(self.data) < pos:
				self.data.append(0)
		self.root_pos_off = len(self.data)
		self.data.extend(b"\0\0\0\0")

	def _append_key(self, key):
		key = key.encode("utf-8")

		if self.skip_duplicate_keys:
			wk = self.written_keys.get(key)
			if wk is not None:
				return wk

		pos = self._append_key_length(self, len(key), 1 if self.aligned else None)
		self.data.extend(key)
		self.data.append(0)

		if self.skip_duplicate_keys:
			self.written_keys[key] = pos

		return pos

	def _align8(self):
		pos = len(self.data)
		if self.aligned:
			pos = roundup(pos, 8)
			while len(self.data) < pos:
				self.data.append(0)
		return pos

	def _write_elem(self, key, vtype, val):
		S = self.stack[-1]
		S.entries.append(Entry(vtype, self._append_key(key) if S.has_keys else 0, val))

	def write_null(self, key):
		self._write_elem(key, TYPE_Null, 0)

	def write_bool(self, key, val):
		self._write_elem(key, TYPE_Bool, 1 if val else 0)
	def write_int32(self, key, val):
		if val < -0x80000000 or val > 0x7fffffff:
			raise EncodingException("value is out of int32 range")
		if val < 0:
			val += 0x100000000
		self._write_elem(key, TYPE_S32, int(val))
	def write_uint32(self, key, val):
		if val < 0 or val > 0xffffffff:
			raise EncodingException("value is out of uint32 range")
		self._write_elem(key, TYPE_U32, int(val))
	def write_float32(self, key, val):
		self._write_elem(key, TYPE_F32, int.from_bytes(PACK_F32.pack(val), "little"))

	def write_int64(self, key, val):
		if val < -0x8000000000000000 or val > 0x7fffffffffffffff:
			raise EncodingException("value is out of int64 range")
		pos = self._align8()
		self.data.extend(int(val).to_bytes(8, "little", signed=True))
		self._write_elem(key, TYPE_S64, pos)
	def write_uint64(self, key, val):
		if val < 0 or val > 0xffffffffffffffff:
			raise EncodingException("value is out of uint64 range")
		pos = self._align8()
		self.data.extend(int(val).to_bytes(8, "little"))
		self._write_elem(key, TYPE_U64, pos)
	def write_float64(self, key, val):
		pos = self._align8()
		self.data.extend(PACK_F64.pack(val))
		self._write_elem(key, TYPE_F64, pos)

	def write_string(self, key, val):
		val = str(val).encode("utf-8")
		pos = self._append_value_length(self, len(val), 1 if self.aligned else None)
		self.data.extend(val)
		self.data.append(0)
		self._write_elem(key, TYPE_String8, pos)
	def write_bytes(self, key, val):
		pos = self._append_value_length(self, len(val), 1 if self.aligned else None)
		self.data.extend(val)
		self._write_elem(key, TYPE_ByteArray, pos)

	def _write_typed_array(self, key, val, vtype, enc, size):
		pos = self._append_value_length(self, len(val), size if self.aligned else None)
		for v in val:
			self.data.extend(enc.pack(v))
		self._write_elem(key, vtype, pos)
	def write_typed_array_int8(self, key, val):
		self._write_typed_array(key, val, TYPE_TypedArrayS8, PACK_S8, 1)
	def write_typed_array_uint8(self, key, val):
		self._write_typed_array(key, val, TYPE_TypedArrayU8, PACK_U8, 1)
	def write_typed_array_int16(self, key, val):
		self._write_typed_array(key, val, TYPE_TypedArrayS16, PACK_S16, 2)
	def write_typed_array_uint16(self, key, val):
		self._write_typed_array(key, val, TYPE_TypedArrayU16, PACK_U16, 2)
	def write_typed_array_int32(self, key, val):
		self._write_typed_array(key, val, TYPE_TypedArrayS32, PACK_S32, 4)
	def write_typed_array_uint32(self, key, val):
		self._write_typed_array(key, val, TYPE_TypedArrayU32, PACK_U32, 4)
	def write_typed_array_int64(self, key, val):
		self._write_typed_array(key, val, TYPE_TypedArrayS64, PACK_S64, 8)
	def write_typed_array_uint64(self, key, val):
		self._write_typed_array(key, val, TYPE_TypedArrayU64, PACK_U64, 8)
	def write_typed_array_float32(self, key, val):
		self._write_typed_array(key, val, TYPE_TypedArrayF32, PACK_F32, 4)
	def write_typed_array_float64(self, key, val):
		self._write_typed_array(key, val, TYPE_TypedArrayF64, PACK_F64, 8)

	def begin_object(self, key):
		self._write_elem(key, TYPE_Object, 0)
		self.stack.append(StagingObject(True))
	def end_object(self):
		pos = self._write_and_remove_top_object()
		self.stack[-1].entries[-1].val = pos

	def begin_array(self, key):
		self._write_elem(key, TYPE_Array, 0)
		self.stack.append(StagingObject(False))
	def end_array(self):
		S = self.stack[-1]
		num = len(S.entries)

		pos = self._append_array_length(self, num, 4 if self.aligned else None)
		for e in S.entries:
			self.data.extend(e.val.to_bytes(4, "little"))
		for e in S.entries:
			self.data.append(e.vtype)

		self.stack.pop()

		self.stack[-1].entries[-1].val = pos

	def _write_and_remove_top_object(self):
		S = self.stack[-1]
		num = len(S.entries)

		pos = self._append_object_size(self, num, 4 if self.aligned else None)
		for e in S.entries:
			self.data.extend(e.key.to_bytes(4, "little"))
		for e in S.entries:
			self.data.extend(e.val.to_bytes(4, "little"))
		for e in S.entries:
			self.data.append(e.vtype)

		self.stack.pop()
		return pos

	def get_encoded(self):
		while len(self.stack) > 1:
			if self.stack[-1].has_keys:
				self.end_object()
			else:
				self.end_array()
		if len(self.stack) == 1:
			root_pos = self._write_and_remove_top_object()
			b = root_pos.to_bytes(4, "little")
			o = self.root_pos_off
			self.data[o + 0] = b[0]
			self.data[o + 1] = b[1]
			self.data[o + 2] = b[2]
			self.data[o + 3] = b[3]
		return self.data

	def _write_object_contents(self, data):
		if isinstance(data, dict):
			for k, v in data.items():
				self.write_variable(k, v)
		else:
			for k, v in vars(data).items():
				self.write_variable(k, v)

	def write_variable(self, key, val):
		if val is None: self.write_null(key); return
		if isinstance(val, bool): self.write_bool(key, val); return
		if isinstance(val, int): self.write_int64(key, val); return
		if isinstance(val, float): self.write_float64(key, val); return
		if isinstance(val, str): self.write_string(key, val); return
		if isinstance(val, (bytes, bytearray)): self.write_bytes(key, val); return
		if isinstance(val, list):
			self.begin_array(key)
			for e in val:
				self.write_variable(None, e)
			self.end_array()
		if isinstance(val, dict):
			self.begin_object(key)
			for k, v in val.items():
				self.write_variable(k, v)
			self.end_object()
		if hasattr(val, "__dict__"):
			self.begin_object(key)
			for k, v in vars(val).items():
				self.write_variable(k, v)
			self.end_object()


def encode(data, **kwargs):
	W = LinearWriter(**kwargs)
	pos = W._write_object_contents(data)
	return W.get_encoded()


class Validator:
	Success = "success"
	ErrorMissingPrefix = "missing_prefix"
	ErrorEOF = "eof"
	ErrorWrongConfig = "wrong_config"
	def __init__(self, *, prefix=b"DATO", config=Config0):
		self.prefix = prefix
		self.identifier = config.identifier
		self.aligned = config.aligned
		self._parse_key_length = config.key_length_encoding.parse
		self._parse_object_size = config.object_size_encoding.parse
		self._parse_array_length = config.array_length_encoding.parse
		self._parse_value_length = config.value_length_encoding.parse

	def validate(self, data):
		if not data.startswith(self.prefix):
			return False, ErrorMissingPrefix
		pos = len(self.prefix)
		if pos + 1 > len(data):
			return False, ErrorEOF
		identifier = data[pos]
		if identifier != self.identifier:
			return False, ErrorWrongConfig
		if self.aligned:
			pos = roundup(pos, 4)
		if pos + 4 > len(data):
			return False, ErrorEOF
		return self._validate_object_ref(data, pos)

	def _validate_object_ref(self, data, pos):
		pos = int.from_bytes(data[pos : pos + 4], "little")
		if pos + 1 > len(data):
			return False, ErrorEOF
		pos = POS

