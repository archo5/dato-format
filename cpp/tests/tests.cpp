
#include "../dato_reader.hpp"
#include "../dato_writer.hpp"

#include <initializer_list>
#include <stdio.h>


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

#define EBCFG0PFX(t) "DATO", 0, 0x11, t, 0

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
	TestBasicStructures();
}
