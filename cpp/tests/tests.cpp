
#include "../dato_reader.hpp"
#include "../dato_writer.hpp"

#include <initializer_list>
#include <stdio.h>


#define V(...) __VA_ARGS__
template <class T, size_t N> constexpr size_t arraysize(T(&)[N]) { return N; }


static void GenEntries(dato::IntMapEntry* entries, std::initializer_list<unsigned> order)
{
	for (unsigned o : order)
	{
		auto& e = *entries++;
		e.key = 0x12345678U + 0x01010101U * o;
		e.value.type = 0x01U * o;
		e.value.pos = 0x23456789U + 0x01010101U * o;
		//printf("%08x %02x %08x\n", e.key, e.value.type, e.value.pos);
	}
}

static bool CheckEntries(dato::IntMapEntry* entries, unsigned count, int line)
{
	bool success = true;
	dato::IntMapEntry* end = entries + count;
	dato::IntMapEntry* pe = nullptr;
	for (dato::IntMapEntry* e = entries; e != end; pe = e++)
	{
		if (e->value.type + 0x78U != (e->key & 0xff) ||
			e->value.pos != e->key - 0x12345678U + 0x23456789U)
		{
			printf("line %d: slicing detected\n", line);
			printf("%08x %02x %08x\n", e->key, e->value.type, e->value.pos);
			success = false;
		}
		if (!pe)
			continue;
		if (pe->key > e->key)
		{
			printf("line %d: wrong element order: [%d]%u > [%d]%u\n",
				line,
				int(pe - entries), unsigned(pe->key),
				int(e - entries), unsigned(e->key));
			success = false;
		}
	}
	return success;
}

void TestSortingInt()
{
	puts("testing sorting (int)");
	using namespace dato;

	TempMem tm;
#define SORT_TEST(type, ...) \
	{ \
		unsigned order[] = { __VA_ARGS__ }; \
		IntMapEntry data[arraysize(order)]; \
		GenEntries(data, { __VA_ARGS__ }); \
		SortEntriesByKeyInt_##type(tm, data, arraysize(data)); \
		CheckEntries(data, arraysize(data), __LINE__); \
	}
	// insertion sort
	SORT_TEST(Insertion, 0, 1);
	SORT_TEST(Insertion, 1, 0);
	SORT_TEST(Insertion, 0, 1, 2);
	SORT_TEST(Insertion, 2, 1, 0);
	SORT_TEST(Insertion, 1, 0, 2);
	SORT_TEST(Insertion, 0, 2, 1);
	SORT_TEST(Insertion, 0, 2, 2);
	SORT_TEST(Insertion, 5, 4, 3, 2, 1);
	SORT_TEST(Insertion, 15, 16, 31, 32);
	// radix sort
	SORT_TEST(Radix, 0, 1, 2);
	SORT_TEST(Radix, 2, 1, 0);
	SORT_TEST(Radix, 1, 0, 2);
	SORT_TEST(Radix, 0, 2, 1);
	SORT_TEST(Radix, 0, 2, 2);
	SORT_TEST(Radix, 5, 4, 3, 2, 1);
#undef SORT_TEST
}


static void CheckSorting(const char* start, int line)
{
	using namespace dato;
	// generate the data
	StringMapEntry entries[100];
	u32 numEntries = 0;
	for (auto* s = start; *s; s += strlen(s) + 1)
	{
		auto& e = entries[numEntries++];
		e.key.pos = s - start - 1;
		e.key.dataPos = s - start;
		e.key.dataLen = strlen(s);
		e.value.pos = MemHash(s, strlen(s));
		e.value.type = e.value.pos * 3;
	}

	SortEntriesByKeyString(start, entries, numEntries);

#if 1
	for (u32 i = 0; i < numEntries; i++)
		printf("%s,", start + entries[i].key.dataPos);
	puts("");
#endif
	for (u32 i = 1; i < numEntries; i++)
	{
		u32 minlen = entries[i - 1].key.dataLen;
		if (minlen > entries[i].key.dataLen)
			minlen = entries[i].key.dataLen;
		int diff = memcmp(start + entries[i - 1].key.dataPos, start + entries[i].key.dataPos, minlen);
		if (diff > 0 ||
			(diff == 0 && entries[i - 1].key.dataLen > entries[i].key.dataLen))
		{
			printf("line %d: wrong element order: [%d]\"%s\" > [%d]\"%s\"\n",
				line,
				int(entries[i - 1].key.dataLen),
				start + entries[i - 1].key.dataPos,
				int(entries[i].key.dataLen),
				start + entries[i].key.dataPos);
		}
	}
}

void TestSortingString()
{
	puts("testing sorting (string)");
	using namespace dato;

#define SORT_TEST(start) CheckSorting(start, __LINE__)
	SORT_TEST("a\0b\0");
	SORT_TEST("b\0a\0");
	SORT_TEST("aa\0b\0");
	SORT_TEST("a\0bb\0");
	SORT_TEST("aa\0a\0");
	SORT_TEST("a\0ab\0");
	SORT_TEST("ab\0a\0");
	SORT_TEST("ab\0aa\0");
	// GLTF
	SORT_TEST("extensionsUsed\0extensionsRequired\0accessors\0animations\0asset\0"
	/**/"buffers\0bufferViews\0cameras\0images\0materials\0meshes\0nodes\0"
	/**/"samplers\0scene\0scenes\0skins\0textures\0extensions\0extras\0"); // root
	SORT_TEST("name\0extensions\0extras\0pbrMetallicRoughness\0normalTexture\0occlusionTexture\0"
	/**/"emissiveTexture\0emissiveFactor\0alphaMode\0alphaCutoff\0doubleSided\0"); // material
#undef SORT_TEST
}


struct EBElement
{
	int size;
	dato::u64 value;
	const char* str;
	EBElement(const char* s) : size(strlen(s)), value(0), str(s) {}
	EBElement(int b) : size(1), value(b), str(nullptr) {}
	EBElement(int s, dato::u64 v) : size(s), value(v), str(nullptr) {}
};

EBElement U(dato::u32 v) { return EBElement(4, v); }
EBElement UU(dato::u64 v) { return EBElement(8, v); }

EBElement F(dato::f32 v)
{
	dato::u32 u;
	memcpy(&u, &v, 4);
	return U(u);
}
EBElement FF(dato::f64 v)
{
	dato::u64 u;
	memcpy(&u, &v, 8);
	return UU(u);
}

static void DumpBuffer(const void* data, size_t size)
{
	auto* mem = (const char*) data;
	for (size_t i = 0; i < size; i++)
	{
		char c = mem[i];
		if (c >= 0x20 && c < 0x7f)
			fputc(c, stdout);
		else
			printf(":%02X", unsigned(dato::u8(c)));
	}
}

static void DumpBufferMismatch(const void* exps, size_t expl, const void* ress, size_t resl, int line)
{
	printf("ERROR (line %d) - buffer mismatch:\n", line);
	printf("expected: ");
	DumpBuffer(exps, expl);
	puts("");
	printf("actual  : ");
	DumpBuffer(ress, resl);
	puts("");
}

#define CHECK_BUF_EQ(exps, expl, ress, resl) \
	if (expl != resl || memcmp(exps, ress, expl)) \
		DumpBufferMismatch(exps, expl, ress, resl, __LINE__);

struct ExpectationBuilder
{
	char bfr[512]; // not expecting this to get too long
	int len = 0;
	ExpectationBuilder(std::initializer_list<EBElement> elements)
	{
		for (const EBElement& e : elements)
		{
			if (e.str)
			{
				memcpy(bfr + len, e.str, e.size);
				len += e.size;
			}
			else
			{
				memcpy(bfr + len, &e.value, e.size);
				len += e.size;
			}
		}
	}
};

#define EB(...) ExpectationBuilder eb({ __VA_ARGS__ })

#define WRTEST(build, expected) \
{ \
	dato::Writer<dato::WriterConfig0> wr; \
	build; \
	expected; \
	CHECK_BUF_EQ(eb.bfr, eb.len, wr.GetData(), wr.GetSize()); \
}

#define EBCFG0PFX(t) "DATO", 0, 0x7, t, 0

void TestBasicStructures()
{
	puts("testing basic structures");

	WRTEST({ wr.SetRoot(wr.WriteNull()); },
		EB(EBCFG0PFX(0), U(0)));
	WRTEST({ wr.SetRoot(wr.WriteBool(false)); },
		EB(EBCFG0PFX(1), U(0)));
	WRTEST({ wr.SetRoot(wr.WriteBool(true)); },
		EB(EBCFG0PFX(1), U(1)));
	// inline numbers
	WRTEST({ wr.SetRoot(wr.WriteS32(-123456789)); },
		EB(EBCFG0PFX(2), U(-123456789)));
	WRTEST({ wr.SetRoot(wr.WriteU32(123456789)); },
		EB(EBCFG0PFX(3), U(123456789)));
	WRTEST({ wr.SetRoot(wr.WriteF32(0.123456f)); },
		EB(EBCFG0PFX(4), F(0.123456f)));
	// referenced numbers
	WRTEST({ wr.SetRoot(wr.WriteS64(-1234567890987654321)); },
		EB(EBCFG0PFX(5), U(16), U(0), UU(-1234567890987654321)));
	WRTEST({ wr.SetRoot(wr.WriteU64(1234567890987654321)); },
		EB(EBCFG0PFX(6), U(16), U(0), UU(1234567890987654321)));
	WRTEST({ wr.SetRoot(wr.WriteF64(0.123456789)); },
		EB(EBCFG0PFX(7), U(16), U(0), FF(0.123456789)));

	// array (generic)
	WRTEST({ wr.SetRoot(wr.WriteArray(nullptr, 0)); },
		EB(EBCFG0PFX(8), U(12), U(0)));
	WRTEST({ auto v = wr.WriteU32(123); wr.SetRoot(wr.WriteArray(&v, 1)); },
		EB(EBCFG0PFX(8), U(12), U(1), U(123), 3));

	// string map
	WRTEST({ wr.SetRoot(wr.WriteStringMap(nullptr, 0)); },
		EB(EBCFG0PFX(9), U(12), U(0)));
	WRTEST({
		dato::StringMapEntry e;
		e.key = wr.WriteStringKey("abc");
		e.value = wr.WriteU32(1234);
		wr.SetRoot(wr.WriteStringMap(&e, 1)); },
		EB(EBCFG0PFX(9), U(20), U(3), "abc", 0, U(1), U(12), U(1234), 3));

	// int map
	WRTEST({ wr.SetRoot(wr.WriteIntMap(nullptr, 0)); },
		EB(EBCFG0PFX(10), U(12), U(0)));
	WRTEST({
		dato::IntMapEntry e;
		e.key = 0xfefdfcfb;
		e.value = wr.WriteU32(12345);
		wr.SetRoot(wr.WriteIntMap(&e, 1)); },
		EB(EBCFG0PFX(10), U(12), U(1), U(0xfefdfcfb), U(12345), 3));
}

void BuildOnlyTest()
{
	dato::Writer<dato::WriterConfig0> wr;
	auto vref = wr.WriteStringMap(nullptr, 0);
	wr.SetRoot(vref);

	dato::UniversalBufferReader r;
	auto root = r.GetRoot().AsStringMap();
	root.FindValueByKey("what");
	for (const auto& it : root)
	{
		it.GetKeyCStr();
		it.GetValue();
		for (const auto& val : it.GetValue().AsArray())
		{
			val.CastToBool();
			val.CastToNumber<dato::s32>();
			val.CastToNumber<dato::u32>();
			val.CastToNumber<dato::f32>();
			val.AsString8();
			val.AsString16();
			val.AsString32();
			val.AsByteArray();
			val.AsVector<dato::u8>()[1];
			val.AsVector<dato::f64>()[2];
			for (const auto& it1 : val.AsVectorArray<dato::u8>())
				it1[1];
			val.AsVectorArray<dato::f64>();
		}
	}
}

int main()
{
	TestSortingInt();
	TestSortingString();
	TestBasicStructures();
}
