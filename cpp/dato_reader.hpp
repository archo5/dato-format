
#pragma once

#include <assert.h>
#include <string.h>


#ifdef _MSC_VER
#  define DATO_FORCEINLINE __forceinline
#else
#  define DATO_FORCEINLINE inline __attribute__((always_inline))
#endif


namespace dato {

typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long long s64;
typedef unsigned long long u64;
typedef float f32;
typedef double f64;

#if DATO_FAST_UNSAFE_READER
// only for use with aligned data or platforms that support unaligned loads (x86/x64/ARM64)
// not guaranteed to work correctly if strict aliasing is enabled in compiler options!
template <class T> DATO_FORCEINLINE static T ReadT(const void* ptr)
{
	return *(const T*)ptr;
}
#else
template <class T> DATO_FORCEINLINE inline T ReadT(const void* ptr)
{
	T v;
	memcpy(&v, ptr, sizeof(T));
	return v;
}
#endif

static const u8 TYPE_Null = 0;
// embedded value types
static const u8 TYPE_Bool = 1;
static const u8 TYPE_S32 = 2;
static const u8 TYPE_U32 = 3;
static const u8 TYPE_F32 = 4;
// external value reference types
// - one value
static const u8 TYPE_S64 = 5;
static const u8 TYPE_U64 = 6;
static const u8 TYPE_F64 = 7;
// - generic containers
static const u8 TYPE_Array = 8;
static const u8 TYPE_Object = 9;
// - raw arrays (identified by purpose)
// (strings contain an extra 0-termination value not included in their size)
static const u8 TYPE_String8 = 10; // ASCII/UTF-8 or other single-byte encoding
static const u8 TYPE_String16 = 11; // likely to be UTF-16
static const u8 TYPE_String32 = 12; // likely to be UTF-32
static const u8 TYPE_ByteArray = 13;
static const u8 TYPE_Vector = 14;
static const u8 TYPE_VectorArray = 15;

static const u8 SUBTYPE_S8 = 0;
static const u8 SUBTYPE_U8 = 1;
static const u8 SUBTYPE_S16 = 2;
static const u8 SUBTYPE_U16 = 3;
static const u8 SUBTYPE_S32 = 4;
static const u8 SUBTYPE_U32 = 5;
static const u8 SUBTYPE_S64 = 6;
static const u8 SUBTYPE_U64 = 7;
static const u8 SUBTYPE_F32 = 8;
static const u8 SUBTYPE_F64 = 9;


template <class T> struct SubtypeInfo;
template <> struct SubtypeInfo<s8> { enum { Subtype = SUBTYPE_S8 }; };
template <> struct SubtypeInfo<u8> { enum { Subtype = SUBTYPE_U8 }; };
template <> struct SubtypeInfo<s16> { enum { Subtype = SUBTYPE_S16 }; };
template <> struct SubtypeInfo<u16> { enum { Subtype = SUBTYPE_U16 }; };
template <> struct SubtypeInfo<s32> { enum { Subtype = SUBTYPE_S32 }; };
template <> struct SubtypeInfo<u32> { enum { Subtype = SUBTYPE_U32 }; };
template <> struct SubtypeInfo<s64> { enum { Subtype = SUBTYPE_S64 }; };
template <> struct SubtypeInfo<u64> { enum { Subtype = SUBTYPE_U64 }; };
template <> struct SubtypeInfo<f32> { enum { Subtype = SUBTYPE_F32 }; };
template <> struct SubtypeInfo<f64> { enum { Subtype = SUBTYPE_F64 }; };


struct Buffer
{
	u32 GetSize();
	void AddZeroesUntil(u32 pos);
	void AddByte(u8 byte);
	void AddMem(const void* mem, u32 size);
	void SetError_ValueOutOfRange();
};

inline u32 RoundUp(u32 x, u32 n)
{
	return (x + n - 1) / n * n;
}

struct EncodingU8
{
	static u32 Write(Buffer& B, u32 val, u32 align)
	{
		if (val < 0 || val > 255)
		{
			B.SetError_ValueOutOfRange();
			return u32(-1);
		}
		u32 pos = B.GetSize();
		if (align != 0)
		{
			pos = RoundUp(pos + 1, align) - 1;
			B.AddZeroesUntil(pos);
		}
		B.AddByte(val);
		return pos;
	}
	static u32 Parse(const char* data, u32& pos)
	{
		return u8(data[pos++]);
	}
};

struct EncodingU16
{
	static u32 Write(Buffer& B, u16 val, u32 align)
	{
		u32 pos = B.GetSize();
		if (align != 0)
		{
			pos = RoundUp(pos + 2, align) - 2;
			B.AddZeroesUntil(pos);
		}
		B.AddMem(&val, 2);
		return pos;
	}
	static u32 Parse(const char* data, u32& pos)
	{
		u32 v = ReadT<u16>(data + pos);
		pos += 2;
		return v;
	}
};

struct EncodingU32
{
	static u32 Write(Buffer& B, u32 val, u32 align)
	{
		u32 pos = B.GetSize();
		if (align != 0)
		{
			pos = RoundUp(pos + 4, align) - 4;
			B.AddZeroesUntil(pos);
		}
		B.AddMem(&val, 4);
		return pos;
	}
	static u32 Parse(const char* data, u32& pos)
	{
		u32 v = ReadT<u32>(data + pos);
		pos += 4;
		return v;
	}
};

struct Config0
{
	static u8 Identifier() { return 0; }
	bool InitForReading(u8 id) { return id == 0; }

	u32 WriteKeyLength(Buffer& B, u32 val, u32 align)
	{ return EncodingU32::Write(B, val, align); }
	u32 ParseKeyLength(const char* data, u32& pos) const
	{ return EncodingU32::Parse(data, pos); }

	u32 WriteObjectSize(Buffer& B, u32 val, u32 align)
	{ return EncodingU32::Write(B, val, align); }
	u32 ParseObjectSize(const char* data, u32& pos) const
	{ return EncodingU32::Parse(data, pos); }

	u32 WriteArrayLength(Buffer& B, u32 val, u32 align)
	{ return EncodingU32::Write(B, val, align); }
	u32 ParseArrayLength(const char* data, u32& pos) const
	{ return EncodingU32::Parse(data, pos); }

	u32 WriteValueLength(Buffer& B, u32 val, u32 align)
	{ return EncodingU32::Write(B, val, align); }
	u32 ParseValueLength(const char* data, u32& pos) const
	{ return EncodingU32::Parse(data, pos); }
};

struct AdaptiveConfig
{
	typedef u32 ParseFunc(const char* data, u32& pos);

	ParseFunc* keyLength = nullptr;
	ParseFunc* objectSize = nullptr;
	ParseFunc* arrayLength = nullptr;
	ParseFunc* valueLength = nullptr;

	// static u8 Identifier() -- cannot be used for serialization
	bool InitForReading(u8 id)
	{
		switch (id)
		{
		case 0:
			keyLength = &EncodingU32::Parse;
			objectSize = &EncodingU32::Parse;
			arrayLength = &EncodingU32::Parse;
			valueLength = &EncodingU32::Parse;
			return true;
		}
		return false;
	}

	u32 ParseKeyLength(const char* data, u32& pos) const { return keyLength(data, pos); }
	u32 ParseObjectSize(const char* data, u32& pos) const { return objectSize(data, pos); }
	u32 ParseArrayLength(const char* data, u32& pos) const { return arrayLength(data, pos); }
	u32 ParseValueLength(const char* data, u32& pos) const { return valueLength(data, pos); }
};

static const u8 FLAG_Aligned = 1 << 0;
static const u8 FLAG_IntegerKeys = 1 << 1;
static const u8 FLAG_SortedKeys = 1 << 2;
static const u8 FLAG_BigEndian = 1 << 3;
static const u8 FLAG_RelativeObjectRefs = 1 << 4;

template <class Config>
struct BufferReader
{
private:
	Config _cfg = {};
	const char* _data = nullptr;
	u32 _len = 0;
	u8 _flags = 0;
	u32 _root = 0;

	template <class T> DATO_FORCEINLINE T RD(u32 pos) const { return ReadT<T>(_data + pos); }

	// compares the bytes until the first 0-char
	bool KeyEquals(u32 kpos, const char* str) const
	{
		_cfg.ParseKeyLength(_data, kpos);
		return strcmp(str, &_data[kpos]) == 0;
	}
	int KeyCompare(u32 kpos, const char* str) const
	{
		_cfg.ParseKeyLength(_data, kpos);
		return strcmp(str, &_data[kpos]);
	}
	// compares the size first, then all of bytes
	bool KeyEquals(u32 kpos, const void* mem, size_t lenMem) const
	{
		u32 len = _cfg.ParseKeyLength(_data, kpos);
		if (len != lenMem)
			return false;
		return memcmp(mem, &_data[kpos], len) == 0;
	}
	int KeyCompare(u32 kpos, const void* mem, size_t lenMem) const
	{
		u32 len = _cfg.ParseKeyLength(_data, kpos);
		u32 testLen = len < lenMem ? len : lenMem;
		if (int bc = memcmp(mem, &_data[kpos], testLen))
			return bc;
		if (lenMem == len)
			return 0;
		return lenMem < len ? -1 : 1;
	}

public:
	struct DynamicAccessor;
	struct ObjectAccessor
	{
		struct Iterator
		{
			const ObjectAccessor* _obj;
			u32 _i;

			DATO_FORCEINLINE const Iterator& operator * () const { return *this; }
			DATO_FORCEINLINE bool operator != (const Iterator& o) const { return _i != o._i; }
			DATO_FORCEINLINE void operator ++ () { ++_i; }

			DATO_FORCEINLINE u32 GetKeyInt() const { return _obj->GetKeyInt(_i); }
			DATO_FORCEINLINE const char* GetKeyCStr() const { return _obj->GetKeyCStr(_i); }

			DATO_FORCEINLINE DynamicAccessor GetValue() const { return _obj->GetValueByIndex(_i); }
		};

		BufferReader* _r;
		u32 _pos;
		u32 _size;
		u32 _objpos;

		ObjectAccessor(BufferReader* r, u32 pos) : _r(r), _pos(pos)
		{
			_objpos = pos;
			_size = r->_cfg.ParseObjectSize(r->_data, _objpos);
		}

		DATO_FORCEINLINE u32 GetSize() const { return _size; }

		Iterator begin() const { return { this, 0 }; }
		Iterator end() const { return { this, _size }; }

		DATO_FORCEINLINE Iterator operator [](size_t i) const
		{
			assert(i < _size);
			return { this, i };
		}

		// retrieving keys
		u32 GetKeyInt(size_t i) const
		{
			assert(i < _size);
			return _r->RD<u32>(_objpos + i * 4);
		}
		const char* GetKeyCStr(size_t i) const
		{
			assert(i < _size);
			u32 kpos = _r->RD<u32>(_objpos + i * 4);
			_r->_cfg.ParseKeyLength(_r->_data, kpos);
			return &_r->_data[kpos];
		}

		// retrieving values
		DynamicAccessor TryGetValueByIndex(size_t i) const
		{
			if (i >= _size)
				return {};
			return GetValueByIndex(i);
		}
		DynamicAccessor GetValueByIndex(size_t i) const
		{
			assert(i < _size);
			u32 vpos = _objpos + _size * 4 + i * 4;
			u32 tpos = _objpos + _size * 8 + i;
			return { _r, _r->RD<u32>(vpos), _r->RD<u8>(tpos) };
		}

		// searching for values
		DynamicAccessor FindValueByStringKey(const char* keyToFind) const
		{
			if (_r->_flags & FLAG_SortedKeys)
			{
				u32 L = 0, R = _size;
				while (L < R)
				{
					u32 M = (L + R) / 2;
					u32 keyM = _r->RD<u32>(_objpos + M * 4);
					int diff = _r->KeyCompare(keyM, keyToFind);
					if (diff == 0)
						return GetValueByIndex(M);
					if (diff < 0)
						R = M;
					else
						L = M + 1;
				}
			}
			else
			{
				for (u32 i = 0; i < _size; i++)
				{
					u32 key = _r->RD<u32>(_objpos + i * 4);
					if (_r->KeyEquals(key, keyToFind))
						return GetValueByIndex(i);
				}
			}
			return {};
		}
		DynamicAccessor FindValueByStringKey(const void* keyToFind, size_t lenKeyToFind) const
		{
			if (_r->_flags & FLAG_SortedKeys)
			{
				u32 L = 0, R = _size;
				while (L < R)
				{
					u32 M = (L + R) / 2;
					u32 keyM = _r->RD<u32>(_objpos + M * 4);
					int diff = _r->KeyCompare(keyM, keyToFind, lenKeyToFind);
					if (diff == 0)
						return GetValueByIndex(M);
					if (diff < 0)
						R = M;
					else
						L = M + 1;
				}
			}
			else
			{
				for (u32 i = 0; i < _size; i++)
				{
					u32 key = _r->RD<u32>(_objpos + i * 4);
					if (_r->KeyEquals(key, keyToFind, lenKeyToFind))
						return GetValueByIndex(i);
				}
			}
			return {};
		}
		DynamicAccessor FindValueByIntKey(u32 keyToFind) const
		{
			if (_r->_flags & FLAG_SortedKeys)
			{
				u32 L = 0, R = _size;
				while (L < R)
				{
					u32 M = (L + R) / 2;
					u32 keyM = _r->RD<u32>(_objpos + i * 4);
					if (keyToFind == keyM)
						return GetValueByIndex(M);
					if (keyToFind < keyM)
						R = M;
					else
						L = M + 1;
				}
			}
			else
			{
				for (u32 i = 0; i < _size; i++)
				{
					u32 key = _r->RD<u32>(_objpos + i * 4);
					if (key == keyToFind)
						return GetValueByIndex(i);
				}
			}
			return {};
		}
	};
	struct ArrayAccessor
	{
		struct Iterator
		{
			const ArrayAccessor* _obj;
			u32 _i;

			DATO_FORCEINLINE DynamicAccessor operator * () const { return _obj->GetValueByIndex(_i); }
			DATO_FORCEINLINE bool operator != (const Iterator& o) const { return _i != o._i; }
			DATO_FORCEINLINE void operator ++ () { ++_i; }
		};

		BufferReader* _r;
		u32 _pos;
		u32 _size;
		u32 _arrpos;

		ArrayAccessor(BufferReader* r, u32 pos) : _r(r), _pos(pos)
		{
			_arrpos = pos;
			_size = r->_cfg.ParseArrayLength(r->_data, _arrpos);
		}

		DATO_FORCEINLINE u32 GetSize() const { return _size; }

		DATO_FORCEINLINE Iterator begin() const { return { this, 0 }; }
		DATO_FORCEINLINE Iterator end() const { return { this, _size }; }

		DATO_FORCEINLINE DynamicAccessor operator [](size_t i) const { return GetValueByIndex(i); }

		// retrieving values
		DynamicAccessor TryGetValueByIndex(size_t i) const
		{
			if (i >= _size)
				return {};
			return GetValueByIndex(i);
		}
		DynamicAccessor GetValueByIndex(size_t i) const
		{
			assert(i < _size);
			u32 vpos = _arrpos + _size * 4 + i * 4;
			u32 tpos = _arrpos + _size * 8 + i;
			return { _r, _r->RD<u32>(vpos), _r->RD<u8>(tpos) };
		}
	};
	template <class T>
	struct TypedArrayAccessor
	{
		struct Iterator
		{
			const T* _ptr;

			DATO_FORCEINLINE T operator * () const { return ReadT<T>(_ptr); }
			DATO_FORCEINLINE bool operator != (const Iterator& o) const { return _ptr != o._ptr; }
			DATO_FORCEINLINE void operator ++ () { ++_ptr; }
		};

		const T* _data;
		u32 _size;

		TypedArrayAccessor(BufferReader* r, u32 pos)
		{
			_size = r->_cfg.ParseValueLength(r->_data, pos);
			_data = (const T*) (r->_data + pos);
		}

		DATO_FORCEINLINE u32 GetSize() const { return _size; }

		DATO_FORCEINLINE Iterator begin() const { return { _data }; }
		DATO_FORCEINLINE Iterator end() const { return { _data + _size }; }

		DATO_FORCEINLINE T operator [](size_t i) const { return ReadT<T>(&_data[i]); }
	};
	struct ByteArrayAccessor : TypedArrayAccessor<char>
	{
		using TypedArrayAccessor<char>::TypedArrayAccessor;
	};
	template <class T> struct StringAccessor : TypedArrayAccessor<T>
	{
		using TypedArrayAccessor<T>::TypedArrayAccessor;
	};
	template <class T>
	struct VectorAccessor
	{
		const T* _data;
		u8 _subtype;
		u8 _elemCount;

		VectorAccessor(BufferReader* r, u32 pos)
		{
			_subtype = r->RD<u8>(pos++);
			assert(_subtype == SubtypeInfo<T>::Subtype);
			_elemCount = r->RD<u8>(pos++);
			_data = (const T*) (r->_data + pos);
		}

		DATO_FORCEINLINE u8 GetElementCount() const { return _elemCount; }

		DATO_FORCEINLINE T operator [](size_t i) const { return ReadT<T>(&_data[i]); }
	};
	template <class T>
	struct VectorArrayAccessor
	{
		struct Iterator
		{
			const T* _ptr;
			u8 elemCount;

			DATO_FORCEINLINE T operator [](size_t i) const
			{
				assert(i < elemCount);
				return ReadT<T>(&_ptr[i]);
			}
			DATO_FORCEINLINE const Iterator& operator * () const { return *this; }
			DATO_FORCEINLINE bool operator != (const Iterator& o) const { return _ptr != o._ptr; }
			DATO_FORCEINLINE void operator ++ () { _ptr += elemCount; }
		};

		const T* _data;
		u8 _subtype;
		u8 _elemCount;
		u32 _size;

		VectorArrayAccessor(BufferReader* r, u32 pos)
		{
			_subtype = r->RD<u8>(pos++);
			assert(_subtype == SubtypeInfo<T>::Subtype);
			_elemCount = r->RD<u8>(pos++);
			_size = r->_cfg.ParseValueLength(r->_data, pos);
			_data = (const T*) (r->_data + pos);
		}

		DATO_FORCEINLINE u8 GetElementCount() const { return _elemCount; }
		DATO_FORCEINLINE u32 GetSize() const { return _size; }

		DATO_FORCEINLINE Iterator begin() const { return { _data }; }
		DATO_FORCEINLINE Iterator end() const { return { _data + _size * _elemCount }; }

		DATO_FORCEINLINE T operator [](size_t i) const { return ReadT<T>(&_data[i]); }
	};
	struct DynamicAccessor
	{
		BufferReader* _r;
		u32 _pos;
		u8 _type;

		DATO_FORCEINLINE DynamicAccessor() : _r(nullptr), _pos(0) { this->_type = TYPE_Null; }
		DATO_FORCEINLINE DynamicAccessor(BufferReader* r, u32 pos, u8 type)
			: _r(r), _pos(pos), _type(type) {}

		DATO_FORCEINLINE bool IsValid() const { return !!_r; }
		DATO_FORCEINLINE operator const void* () const { return _r; } // to support `if (init)` exprs

		// reading type info
		DATO_FORCEINLINE u8 GetType() const { return _type; }
		DATO_FORCEINLINE bool IsNull() const { return _type == TYPE_Null; }
		DATO_FORCEINLINE u8 GetSubtype() const
		{
			assert(_type == TYPE_Vector || TYPE_VectorArray);
			return _r->RD<u8>(_pos);
		}
		DATO_FORCEINLINE u8 GetElementCount() const
		{
			assert(_type == TYPE_Vector || TYPE_VectorArray);
			return _r->RD<u8>(_pos + 1);
		}

		// type checks for more complex types
		DATO_FORCEINLINE bool IsVector(u8 subtype, u8 elemCount) const
		{
			return _type == TYPE_Vector
				&& _r->RD<u8>(_pos) == subtype
				&& _r->RD<u8>(_pos + 1) == elemCount;
		}
		template<class T> DATO_FORCEINLINE bool IsVectorT(u8 elemCount) const
		{
			return IsVector(SubtypeInfo<T>::Subtype, elemCount);
		}
		DATO_FORCEINLINE bool IsVectorArray(u8 subtype, u8 elemCount) const
		{
			return _type == TYPE_VectorArray
				&& _r->RD<u8>(_pos) == subtype
				&& _r->RD<u8>(_pos + 1) == elemCount;
		}
		template<class T> DATO_FORCEINLINE bool IsVectorArrayT(u8 elemCount) const
		{
			return IsVectorArray(SubtypeInfo<T>::Subtype, elemCount);
		}

		// reading the exact data
		DATO_FORCEINLINE bool AsBool() const { assert(_type == TYPE_Bool); return _pos != 0; }
		DATO_FORCEINLINE s32 AsS32() const { assert(_type == TYPE_S32); return _pos; }
		DATO_FORCEINLINE u32 AsU32() const { assert(_type == TYPE_U32); return _pos; }
		DATO_FORCEINLINE float AsF32() const { assert(_type == TYPE_U32); return ReadT<float>(&_pos); }
		DATO_FORCEINLINE s64 AsS64() const { assert(_type == TYPE_S64); return _r->RD<s64>(_pos); }
		DATO_FORCEINLINE u64 AsU64() const { assert(_type == TYPE_U64); return _r->RD<u64>(_pos); }
		DATO_FORCEINLINE double AsF64() const { assert(_type == TYPE_F64); return _r->RD<double>(_pos); }

		DATO_FORCEINLINE ObjectAccessor AsObject() const
		{
			assert(_type == TYPE_Object);
			return { _r, _pos };
		}
		DATO_FORCEINLINE ArrayAccessor AsArray() const
		{
			assert(_type == TYPE_Array);
			return { _r, _pos };
		}

		DATO_FORCEINLINE StringAccessor<char> AsString8() const
		{
			assert(_type == TYPE_String8);
			return { _r, _pos };
		}
		DATO_FORCEINLINE StringAccessor<u16> AsString16() const
		{
			assert(_type == TYPE_String16);
			return { _r, _pos };
		}
		DATO_FORCEINLINE StringAccessor<u32> AsString32() const
		{
			assert(_type == TYPE_String32);
			return { _r, _pos };
		}
		DATO_FORCEINLINE ByteArrayAccessor AsByteArray() const
		{
			assert(_type == TYPE_ByteArray);
			return { _r, _pos };
		}

		template <class T> DATO_FORCEINLINE VectorAccessor<T> AsVector() const
		{
			return { _r, _pos };
		}
		template <class T> DATO_FORCEINLINE VectorArrayAccessor<T> AsVectorArray() const
		{
			return { _r, _pos };
		}

		// casts
		template <class T> T CastToNumber() const
		{
			switch (_type)
			{
			case TYPE_Bool: return T(_pos ? 1 : 0);
			case TYPE_S32: return T(s32(_pos));
			case TYPE_U32: return T(_pos);
			case TYPE_F32: return T(ReadT<float>(&_pos));
			case TYPE_S64: return T(_r->RD<s64>(_pos));
			case TYPE_U64: return T(_r->RD<u64>(_pos));
			case TYPE_F64: return T(_r->RD<double>(_pos));
			default: return T(0);
			}
		}
		bool CastToBool() const
		{
			switch (_type)
			{
			case TYPE_Bool:
			case TYPE_S32:
			case TYPE_U32: return _pos != 0;
			case TYPE_F32: return ReadT<float>(&_pos) != 0;
			case TYPE_S64:
			case TYPE_U64: return _r->RD<u64>(_pos) != 0;
			case TYPE_F64: return _r->RD<double>(_pos) != 0;
			default: return false;
			}
		}
	};

	bool Init(const void* data, u32 len, const void* prefix, u32 prefix_len, u8 ignore_flags)
	{
		if (prefix_len + 2 > len)
			return false;

		if (0 != memcmp(data, prefix, prefix_len))
			return false;

		const char* cdata = (const char*) data;
		u32 rootpos = prefix_len + 2;
		if (cdata[prefix_len + 1] & FLAG_Aligned)
			rootpos = RoundUp(rootpos, 4);
		if (rootpos + 4 > len)
			return false;

		u32 root = ReadT<u32>(cdata + rootpos);
		if (root + 4 > len)
			return false;

		if (!_cfg.InitForReading(cdata[prefix_len]))
			return false;

		// safe to init
		_data = cdata;
		_len = len;
		_flags = cdata[prefix_len + 1] & ~ignore_flags;
		_root = root;
		return true;
	}

	ObjectAccessor GetRoot()
	{
		return { this, _root };
	}
};

typedef BufferReader<Config0> Config0BufferReader;
typedef BufferReader<AdaptiveConfig> UniversalBufferReader;

} // dato
