
#pragma once

#include <assert.h>
#include <string.h>
#include <malloc.h>
#ifdef DATO_USE_STD_SORT // increases compile time, only valuable as a reference/workaround
#include <algorithm>
#endif


#ifdef _MSC_VER
#  define DATO_FORCEINLINE __forceinline
#else
#  define DATO_FORCEINLINE inline __attribute__((always_inline))
#endif


namespace dato {

#ifndef DATO_COMMON_DEFS
#define DATO_COMMON_DEFS

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

static const u8 FLAG_Aligned = 1 << 0;
static const u8 FLAG_IntegerKeys = 1 << 1;
static const u8 FLAG_SortedKeys = 1 << 2;
static const u8 FLAG_BigEndian = 1 << 3;
static const u8 FLAG_RelativeObjectRefs = 1 << 4;

// override this if you're adding inline types
#ifndef DATO_IS_REFERENCE_TYPE
#define DATO_IS_REFERENCE_TYPE(t) ((t) >= TYPE_S64)
#endif

DATO_FORCEINLINE bool IsReferenceType(u8 t)
{
	return DATO_IS_REFERENCE_TYPE(t);
}

inline u32 RoundUp(u32 x, u32 n)
{
	return (x + n - 1) / n * n;
}

#endif // DATO_COMMON_DEFS


#if DATO_FAST_UNSAFE
template <class Dst, class Src> DATO_FORCEINLINE Dst BitCast(Src v)
{
	static_assert(sizeof(Dst) == sizeof(Src), "sizes don't match");
	return *(Dst*)&v;
}
#else
template <class Dst, class Src> DATO_FORCEINLINE Dst BitCast(Src v)
{
	static_assert(sizeof(Dst) == sizeof(Src), "sizes don't match");
	Dst ret;
	memcpy(&ret, &v, sizeof(v));
	return ret;
}
#endif

template <class T> inline u32 StrLen(const T* str)
{
	const T* p = str;
	while (*p)
		p++;
	return p - str;
}

struct Builder
{
	char* _data = nullptr;
	u32 _size = 0;
	u32 _mem = 0;

	~Builder()
	{
		free(_data);
	}

	DATO_FORCEINLINE const void* GetData() const { return _data; }
	DATO_FORCEINLINE u32 GetSize() const { return _size; }

	void Reserve(u32 atLeast)
	{
		if (_size < atLeast)
			_ResizeImpl(atLeast);
	}

	void _ResizeImpl(u32 newSize)
	{
		_data = (char*) realloc(_data, newSize);
		_mem = newSize;
	}
	void _ReserveForAppend(u32 sizeToAppend)
	{
		if (_size + sizeToAppend > _mem)
			_ResizeImpl(_size + sizeToAppend + _mem);
	}

	void AddZeroes(u32 num)
	{
		_ReserveForAppend(num);
		for (u32 i = 0; i < num; i++)
			_data[_size++] = 0;
	}
	void AddZeroesUntil(u32 pos)
	{
		if (pos <= _size)
			return;
		_ReserveForAppend(pos - _size);
		while (_size < pos)
			_data[_size++] = 0;
	}
	DATO_FORCEINLINE void AddU32(u32 v)
	{
		AddMem(&v, 4);
	}
	void AddByte(u8 byte)
	{
		_ReserveForAppend(1);
		_data[_size++] = char(byte);
	}
	void AddMem(const void* mem, u32 size)
	{
		_ReserveForAppend(size);
		memcpy(&_data[_size], mem, size);
		_size += size;
	}

	void SetError_ValueOutOfRange();
};

inline u32 WriteSizeU8(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
{
	if (val > 0xff)
	{
		B.SetError_ValueOutOfRange();
		return u32(-1);
	}
	u32 pos = B.GetSize();
	if (align != 0)
	{
		u32 totalsize = 1 + pfxsize;
		pos = RoundUp(pos + totalsize, align) - totalsize;
		B.AddZeroesUntil(pos);
	}
	if (pfxsize)
		B.AddMem(prefix, pfxsize);
	B.AddByte(val);
	return pos;
}

inline u32 WriteSizeU16(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
{
	if (val > 0xffff)
	{
		B.SetError_ValueOutOfRange();
		return u32(-1);
	}
	u32 pos = B.GetSize();
	if (align != 0)
	{
		u32 totalsize = 2 + pfxsize;
		pos = RoundUp(pos + totalsize, align) - totalsize;
		B.AddZeroesUntil(pos);
	}
	if (pfxsize)
		B.AddMem(prefix, pfxsize);
	B.AddMem(&val, 2);
	return pos;
}

inline u32 WriteSizeU32(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
{
	u32 pos = B.GetSize();
	if (align != 0)
	{
		u32 totalsize = 4 + pfxsize;
		pos = RoundUp(pos + totalsize, align) - totalsize;
		B.AddZeroesUntil(pos);
	}
	if (pfxsize)
		B.AddMem(prefix, pfxsize);
	B.AddMem(&val, 4);
	return pos;
}

inline u32 WriteSizeU8X32(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
{
	u32 pos = B.GetSize();
	if (val < 0xff)
	{
		if (align != 0)
		{
			u32 totalsize = 1 + pfxsize;
			pos = RoundUp(pos + totalsize, align) - totalsize;
			B.AddZeroesUntil(pos);
		}
		if (pfxsize)
			B.AddMem(prefix, pfxsize);
		B.AddByte(val);
	}
	else
	{
		if (align != 0)
		{
			u32 totalsize = 5 + pfxsize;
			pos = RoundUp(pos + totalsize, align) - totalsize;
			B.AddZeroesUntil(pos);
		}
		if (pfxsize)
			B.AddMem(prefix, pfxsize);
		B.AddByte(0xff);
		B.AddMem(&val, 4);
	}
	return pos;
}

struct WriterConfig0
{
	static u8 Identifier() { return 0; }

	static u32 WriteKeyLength(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU32(B, val, align, prefix, pfxsize); }
	static u32 WriteObjectSize(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU32(B, val, align, prefix, pfxsize); }
	static u32 WriteArrayLength(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU32(B, val, align, prefix, pfxsize); }
	static u32 WriteValueLength(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU32(B, val, align, prefix, pfxsize); }
};

struct WriterConfig1
{
	static u8 Identifier() { return 1; }

	static u32 WriteKeyLength(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU32(B, val, align, prefix, pfxsize); }
	static u32 WriteObjectSize(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU32(B, val, align, prefix, pfxsize); }
	static u32 WriteArrayLength(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU32(B, val, align, prefix, pfxsize); }
	static u32 WriteValueLength(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU8X32(B, val, align, prefix, pfxsize); }
};

struct WriterConfig2
{
	static u8 Identifier() { return 2; }

	static u32 WriteKeyLength(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU8X32(B, val, align, prefix, pfxsize); }
	static u32 WriteObjectSize(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU8X32(B, val, align, prefix, pfxsize); }
	static u32 WriteArrayLength(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU8X32(B, val, align, prefix, pfxsize); }
	static u32 WriteValueLength(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU8X32(B, val, align, prefix, pfxsize); }
};

struct WriterConfig3
{
	static u8 Identifier() { return 3; }

	static u32 WriteKeyLength(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU8(B, val, align, prefix, pfxsize); }
	static u32 WriteObjectSize(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU8(B, val, align, prefix, pfxsize); }
	static u32 WriteArrayLength(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU32(B, val, align, prefix, pfxsize); }
	static u32 WriteValueLength(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU32(B, val, align, prefix, pfxsize); }
};

struct WriterConfig4
{
	static u8 Identifier() { return 4; }

	static u32 WriteKeyLength(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU8(B, val, align, prefix, pfxsize); }
	static u32 WriteObjectSize(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU8(B, val, align, prefix, pfxsize); }
	static u32 WriteArrayLength(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU8X32(B, val, align, prefix, pfxsize); }
	static u32 WriteValueLength(Builder& B, u32 val, u32 align, const void* prefix, u32 pfxsize)
	{ return WriteSizeU8X32(B, val, align, prefix, pfxsize); }
};

struct KeyRef
{
	u32 pos;
	u32 dataPos;
	u32 dataLen;
};

struct ValueRef
{
	u8 type;
	u32 pos;
};

struct EntryRef
{
	KeyRef key;
	ValueRef value;
};

inline u32 MemHash(const void* rawp, u32 len)
{
	auto* mem = (const char*) rawp;
	u32 nToHash = len < 32 ? len : 32;
	u32 interval = len / nToHash;
	u32 hash = 0x811c9dc5;
	for (u32 i = 0; i < len; i += interval)
	{
		hash ^= mem[i];
		hash *= 0x01000193;
	}
	return hash;
}

struct MemReuseHashTable
{
	struct Entry
	{
		u32 valuePos;
		u32 dataOff;
		u32 len;
		u32 hash;
	};

	char** _pdata = nullptr; // must be initialized
	Entry* _entries = nullptr;
	u32 _numEntries = 0;
	u32 _memEntries = 0;
	static constexpr const u32 NO_VALUE = 0xffffffff;
	u32* _table = nullptr;
	u32 _numTableSlots = 0;

	MemReuseHashTable(char*& data) : _pdata(&data) {}
	~MemReuseHashTable()
	{
		free(_entries);
		free(_table);
	}

	Entry* Find(const void* mem, u32 len) const
	{
		char* data = *_pdata;
		u32 hash = MemHash(mem, len);
		u32 ipos = hash % _numTableSlots;
		u32 pos = ipos;
		for (;;)
		{
			u32 p = _table[pos];
			if (p == NO_VALUE)
				return nullptr;
			Entry& e = _entries[p];
			if (e.hash == hash &&
				e.len == len &&
				memcmp(&data[e.dataOff], mem, len) == 0)
				return &e;
			pos = (pos + 1) % _numTableSlots;
			if (pos == ipos)
				return nullptr;
		}
	}

	// must not already exist in the table
	void Insert(u32 valuePos, u32 dataOff, u32 len)
	{
		if (_numEntries * 5 >= _numTableSlots * 4)
			_Rehash(_numTableSlots == 0 ? 16 : _numTableSlots * 2);

		if (_numEntries >= _memEntries)
		{
			_memEntries = _memEntries == 0 ? 16 : _memEntries;
			_entries = (Entry*) realloc(_entries, _memEntries * sizeof(Entry));
		}

		char* data = *_pdata;
		u32 hash = MemHash(&data[dataOff], len);
		u32 entryIndex = _numEntries++;
		_entries[entryIndex] = { valuePos, dataOff, len, hash };
		_Insert(hash, entryIndex);
	}

	void _Rehash(u32 newSlots)
	{
		_table = (u32*) realloc(_table, newSlots * sizeof(u32));
		_numTableSlots = newSlots;

		for (u32 i = 0; i < newSlots; i++)
			_table[i] = NO_VALUE;

		for (u32 i = 0; i < _numEntries; i++)
		{
			const Entry& e = _entries[i];
			_Insert(e.hash, i);
		}
	}

	void _Insert(u32 hash, u32 entryIndex)
	{
		u32 ipos = hash % _numTableSlots;
		u32 pos = ipos;
		for (;;)
		{
			u32 p = _table[pos];
			if (p == NO_VALUE)
			{
				_table[pos] = entryIndex;
				return;
			}

			// TODO robin hood hashing

			pos = (pos + 1) % _numTableSlots;
			if (pos == ipos)
			{
				assert(!"unexpected failure to insert");
				return;
			}
		}
	}
};

struct WriterBase : Builder
{
	MemReuseHashTable _keyTable { _data };
	u32 _rootPos;
	u8 _flags;

	WriterBase(const char* prefix, u32 pfxsize, u8 cfgid, u8 flags) : _flags(flags)
	{
		AddMem(prefix, pfxsize);
		AddByte(cfgid);
		AddByte(flags);

		// root position
		if (flags & FLAG_Aligned)
			AddZeroesUntil(RoundUp(GetSize(), 4));
		_rootPos = GetSize();
		AddZeroesUntil(GetSize() + 4); // reserve the space
	}

	void SetRoot(u32 pos)
	{
		memcpy(&_data[_rootPos], &pos, 4);
	}

	DATO_FORCEINLINE u8 Align(u8 a)
	{
		return _flags & FLAG_Aligned ? a : 0;
	}

	DATO_FORCEINLINE KeyRef WriteIntKey(u32 k)
	{
		return { k, 0, 0 };
	}

	DATO_FORCEINLINE u32 AddValue8(const void* mem)
	{
		u32 ret = GetSize();
		if (_flags & FLAG_Aligned)
			AddZeroesUntil(RoundUp(ret, 8));
		AddMem(mem, 8);
		return ret;
	}

	DATO_FORCEINLINE ValueRef WriteNull()
	{
		return { TYPE_Null, 0 };
	}
	DATO_FORCEINLINE ValueRef WriteBool(bool v)
	{
		return { TYPE_Bool, u32(v) };
	}
	DATO_FORCEINLINE ValueRef WriteS32(s32 v)
	{
		return { TYPE_S32, u32(v) };
	}
	DATO_FORCEINLINE ValueRef WriteU32(u32 v)
	{
		return { TYPE_U32, v };
	}
	DATO_FORCEINLINE ValueRef WriteF32(f32 v)
	{
		return { TYPE_U32, BitCast<u32>(v) };
	}
	DATO_FORCEINLINE ValueRef WriteS64(s64 v)
	{
		return { TYPE_S64, AddValue8(&v) };
	}
	DATO_FORCEINLINE ValueRef WriteU64(u64 v)
	{
		return { TYPE_S64, AddValue8(&v) };
	}
	DATO_FORCEINLINE ValueRef WriteF64(u64 v)
	{
		return { TYPE_S64, AddValue8(&v) };
	}

	ValueRef WriteVectorRaw(const void* data, u8 subtype, u8 sizeAlign, u8 elemCount)
	{
		AddZeroesUntil(RoundUp(GetSize() + 2, sizeAlign) - 2);
		u32 pos = GetSize();
		AddByte(subtype);
		AddByte(elemCount);
		AddMem(data, sizeAlign * elemCount);
		return { TYPE_Vector, pos };
	}
	template <class T>
	DATO_FORCEINLINE ValueRef WriteVectorT(const T* values, u8 elemCount)
	{
		return WriteVectorRaw(values, SubtypeInfo<T>::Subtype, sizeof(T), elemCount);
	}
};

template <class T>
struct TempMem
{
	T* data = nullptr;
	u32 size = 0;

	~TempMem()
	{
		free(data);
	}
	T* GetData(u32 atLeast)
	{
		if (size < atLeast)
		{
			size += atLeast;
			free(data);
			data = (T*) malloc(sizeof(T) * size);
		}
		return data;
	}
	T* CopyData(const T* from, u32 count)
	{
		T* data = GetData(count);
		memcpy(data, from, sizeof(T) * count);
		return data;
	}
};

#ifndef DATO_USE_STD_SORT
template <class T> DATO_FORCEINLINE void PODSwap(T& a, T& b)
{
	T tmp = a;
	a = b;
	b = tmp;
}

inline void SortEntriesByKeyInt(TempMem<EntryRef>& tempMem, EntryRef* entries, u32 count)
{
	if (count <= 16)
	{
		// insertion sort
		for (u32 i = 1; i < count; i++)
		{
			for (u32 j = i; j > 0 && entries[j - 1].key.pos > entries[j].key.pos; j--)
			{
				PODSwap(entries[j - 1], entries[j]);
			}
		}
		return;
	}

	// radix sort
	EntryRef* from = entries;
	EntryRef* to = tempMem.GetData(count);
	for (int part = 0; part < 4; part++)
	{
		u32 shift = part * 8;

		// count the number of elements in each bucket
		u32 numElements[256] = {};
		for (u32 i = 0; i < count; i++)
			numElements[(from[i].key.pos >> shift) & 0xff]++;

		// allocate ranges for each bucket
		u32 offsets[256];
		u32 o = 0;
		for (u32 i = 0; i < 256; i++)
		{
			offsets[i] = o;
			o += numElements[i];
		}

		// copy elements according to their assigned offsets
		for (u32 i = 0; i < count; i++)
			to[offsets[(from[i].key.pos >> shift) & 0xff]++] = from[i];

		PODSwap(from, to);
	}
}

inline int Q3SS_CharAt(const char* mem, const EntryRef& e, u32 at)
{
	if (at < e.key.dataLen)
		return u8(mem[e.key.dataPos + at]);
	return -1;
}

// three-way string quicksort
inline void Quick3StringSort(const char* mem, EntryRef* entries, int low, int high, u32 which)
{
	if (high <= low)
		return;

	int sublow = low;
	int subhigh = high;
	int ch = Q3SS_CharAt(mem, entries[low], which);
	for (int i = low + 1; i <= subhigh; )
	{
		int nch = Q3SS_CharAt(mem, entries[i], which);
		if (nch < ch)
			PODSwap(entries[sublow++], entries[i++]);
		else if (nch > ch)
			PODSwap(entries[i], entries[subhigh--]);
		else
			i++;
	}
	Quick3StringSort(mem, entries, low, sublow - 1, which);
	if (ch >= 0)
		Quick3StringSort(mem, entries, sublow, subhigh, which + 1);
	Quick3StringSort(mem, entries, subhigh + 1, high, which);
}

inline void SortEntriesByKeyString(const char* mem, EntryRef* entries, u32 count)
{
	Quick3StringSort(mem, entries, 0, int(count - 1), 0);
}
#endif // DATO_USE_STD_SORT

template <class Config>
struct Writer : WriterBase
{
	TempMem<EntryRef> _sortableEntries;
	TempMem<EntryRef> _sortCopyEntries;
	bool _skipDuplicateKeys;

	DATO_FORCEINLINE Writer
	(
		const char* prefix = "DATO",
		u32 pfxsize = 4,
		u8 flags = FLAG_Aligned | FLAG_RelativeObjectRefs,
		bool skipDuplicateKeys = true
	)
		: WriterBase(prefix, pfxsize, Config::Identifier(), flags)
		, _skipDuplicateKeys(skipDuplicateKeys)
	{}

	KeyRef WriteStringKey(const char* str, u32 size)
	{
		if (_skipDuplicateKeys)
		{
			if (auto* e = _keyTable.Find(str, size))
				return { e->valuePos, e->dataPos, e->len };
		}

		u32 pos = Config::WriteKeyLength(*this, size, 0, nullptr, 0);
		u32 dataPos = GetSize();
		AddMem(str, size);
		AddByte(0);

		if (_skipDuplicateKeys)
		{
			_keyTable.Insert(pos, dataPos, size);
		}

		return { pos, dataPos, size };
	}
	DATO_FORCEINLINE KeyRef WriteStringKey(const char* str) { return WriteStringKey(str, StrLen(str)); }

	ValueRef WriteObject(const EntryRef* entries, u32 count)
	{
		if (_flags & FLAG_SortedKeys)
		{
			EntryRef* sea = _sortableEntries.CopyData(entries, count);
			_SortObjectEntries(sea, count);
			entries = sea;
		}
		return _WriteObjectImpl(entries, count);
	}

	ValueRef WriteObjectInlineSort(EntryRef* entries, u32 count)
	{
		if (_flags & FLAG_SortedKeys)
		{
			_SortObjectEntries(entries, count);
		}
		return _WriteObjectImpl(entries, count);
	}

	void _SortObjectEntries(EntryRef* entries, u32 count)
	{
		if (_flags & FLAG_IntegerKeys)
		{
#ifdef DATO_USE_STD_SORT
			std::sort(
				entries,
				entries + count,
				[](const EntryRef& a, const EntryRef& b)
			{
				return a.key.pos < b.key.pos;
			});
#else
			SortEntriesByKeyInt(_sortCopyEntries, entries, count);
#endif
		}
		else
		{
#ifdef DATO_USE_STD_SORT
			std::sort(
				entries,
				entries + count,
				[this](const EntryRef& a, const EntryRef& b)
			{
				u32 minSize = a.key.dataLen < b.key.dataLen ? a.key.dataLen : b.key.dataLen;
				const char* ka = _data + a.key.dataPos;
				const char* kb = _data + b.key.dataPos;
				if (int diff = memcmp(ka, kb, minSize))
					return diff < 0;
				return a.key.dataLen < b.key.dataLen;
			});
#else
			SortEntriesByKeyString(_data, entries, count);
#endif
		}
	}

	ValueRef _WriteObjectImpl(const EntryRef* entries, u32 count)
	{
		u32 pos = Config::WriteObjectSize(*this, count, Align(4), nullptr, 0);
		u32 objpos = GetSize();
		for (u32 i = 0; i < count; i++)
			AddU32(entries[i].key.pos);
		if (_flags & FLAG_RelativeObjectRefs)
		{
			for (u32 i = 0; i < count; i++)
			{
				u32 vp = entries[i].value.pos;
				if (IsReferenceType(entries[i].value.type))
					vp -= objpos;
				AddU32(vp);
			}
		}
		else
		{
			for (u32 i = 0; i < count; i++)
				AddU32(entries[i].value.pos);
		}
		for (u32 i = 0; i < count; i++)
			AddByte(entries[i].value.type);
		return { TYPE_Object, pos };
	}

	ValueRef WriteArray(const ValueRef* values, u32 count)
	{
		u32 pos = Config::WriteArrayLength(*this, count, Align(4), nullptr, 0);
		u32 objpos = GetSize();
		if (_flags & FLAG_RelativeObjectRefs)
		{
			for (u32 i = 0; i < count; i++)
			{
				u32 vp = values[i].pos;
				if (IsReferenceType(entries[i].type))
					vp -= objpos;
				AddU32(vp);
			}
		}
		else
		{
			for (u32 i = 0; i < count; i++)
				AddU32(values[i].pos);
		}
		for (u32 i = 0; i < count; i++)
			AddByte(values[i].type);
		return pos;
	}

	ValueRef WriteString8(const char* str, u32 size)
	{
		u32 pos = Config::WriteValueLength(*this, size, 0, nullptr, 0);
		AddMem(str, size);
		AddByte(0);
		return { TYPE_String8, pos };
	}
	DATO_FORCEINLINE ValueRef WriteString8(const char* str) { return WriteString8(str, StrLen(str)); }

	ValueRef WriteString16(const u16* str, u32 size)
	{
		u32 pos = Config::WriteValueLength(*this, size, Align(2), nullptr, 0);
		AddMem(str, size * sizeof(*str));
		AddZeroes(2);
		return { TYPE_String16, pos };
	}
	DATO_FORCEINLINE ValueRef WriteString16(const char* str) { return WriteString16(str, StrLen(str)); }

	ValueRef WriteString32(const u32* str, u32 size)
	{
		u32 pos = Config::WriteValueLength(*this, size, Align(4), nullptr, 0);
		AddMem(str, size * sizeof(*str));
		AddZeroes(4);
		return { TYPE_String32, pos };
	}
	DATO_FORCEINLINE ValueRef WriteString32(const char* str) { return WriteString32(str, StrLen(str)); }

	ValueRef WriteByteArray(const void* data, u32 size, u32 align = 0)
	{
		u32 pos = Config::WriteValueLength(*this, size, align, nullptr, 0);
		AddMem(data, size);
		return { TYPE_ByteArray, pos };
	}

	ValueRef WriteVectorArrayRaw(const void* data, u8 subtype, u8 sizeAlign, u8 elemCount, u32 length)
	{
		u8 prefix[] = { subtype, elemCount };
		u32 pos = Config::WriteValueLength(*this, length, Align(sizeAlign), prefix, sizeof(prefix));
		AddMem(values, sizeAlign * elemCount * length);
		return pos;
	}
	template <class T>
	DATO_FORCEINLINE ValueRef WriteVectorArrayT(const T* values, u8 elemCount, u32 length)
	{
		return WriteVectorArrayRaw(values, SubtypeInfo<T>::Subtype, sizeof(T), elemCount, length);
	}
};

} // dato
