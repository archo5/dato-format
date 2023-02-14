
#define _HAS_EXCEPTIONS 0
#define _CRT_SECURE_NO_WARNINGS
#include "../dato_reader.hpp"
#include "../dato_writer.hpp"
#include "../dato_dump.hpp"

#include "bench.hpp"

#include <stdlib.h>
#include <vector>

using namespace dato;


#ifndef CONFIG
#define CONFIG 0
#endif

#define _DATO_CONCAT(a, b) a ## b
#define DATO_CONCAT(a, b) _DATO_CONCAT(a, b)
#define _DATO_STRINGIFY(a) #a
#define DATO_STRINGIFY(a) _DATO_STRINGIFY(a)
#define DATO_CONFIG DATO_CONCAT(WriterConfig, CONFIG)
using WRTR = Writer<DATO_CONFIG>;
using RDR = Reader<DATO_CONCAT(ReaderConfig, CONFIG)>;
//using RDR = Reader<ReaderAdaptiveConfig>;


static bool streq(const char* a, const char* b)
{
	for (;;)
	{
		if (*a != *b)
			return false;
		if (*a == 0) // implied *b == 0
			return true;
		a++;
		b++;
	}
}

struct LCG
{
	unsigned state = 12345;

	unsigned get()
	{
		return state = state * 1664525 + 1013904223;
	}
	float getf()
	{
		return double(get()) / 0xffffffff;
	}
};

void SaveBuffer(const char* filename, WRTR& W)
{
	FILE* fp = fopen(filename, "wb");
	fwrite(W.GetData(), W.GetSize(), 1, fp);
	fclose(fp);
}

struct NULLValueIterator : IValueIterator
{
	void UseMem(const void* mem, size_t size)
	{
		auto* m = (const char*) mem;
		u8 x = 0;
		for (size_t i = 0; i < size; i++)
			x += m[i];
		DoNotOpt(x);
	}

	void BeginMap(u8 type, u32 size) override {}
	void EndMap(u8 type) override {}
	void BeginStringKey(const char* key, u32 length) override
	{
		UseMem(key, length);
	}
	void EndStringKey() override {}
	void BeginIntKey(u32 key) override {}
	void EndIntKey() override {}

	void BeginArray(u32 size) override {}
	void EndArray() override {}
	void BeginArrayIndex(u32 i) override {}
	void EndArrayIndex() override {}

	void OnValueNull() override {}
	void OnValueBool(bool value) override {}
	void OnValueS32(s32 value) override {}
	void OnValueU32(u32 value) override {}
	void OnValueF32(f32 value) override {}
	void OnValueS64(s64 value) override {}
	void OnValueU64(u64 value) override {}
	void OnValueF64(f64 value) override {}
	void OnValueString(const char* data, u32 length) override
	{
		UseMem(data, length);
	}
	void OnValueString(const u16* data, u32 length) override
	{
		UseMem(data, length * 2);
	}
	void OnValueString(const u32* data, u32 length) override
	{
		UseMem(data, length * 4);
	}
	void OnValueByteArray(const void* data, u32 length) override
	{
		UseMem(data, length);
	}
	void OnValueVector(u8 subtype, u8 elemCount, const void* data) override
	{
		UseMem(data, elemCount * SubtypeGetSize(subtype));
	}
	void OnValueVectorArray(u8 subtype, u8 elemCount, const void* data, u32 length) override
	{
		UseMem(data, elemCount * length * SubtypeGetSize(subtype));
	}
	void OnUnknownValue(u8 type, u32 embedded, const char* buffer, u32 length) override {}
};


static void gen_nodes(int argc, char* argv[])
{
	int count = 1000;

	WRTR W;
	{
		Benchmark B("gen-nodes");//, 100000, 10);
		while (B.Iterate())
		{
			LCG lcg;
			W.~WRTR();
			new (&W) WRTR("DATO", 4, FLAG_Aligned | FLAG_SortedKeys, true);
			W.Reserve(1024 * 1024);

			std::vector<ValueRef> vrnodes;
			vrnodes.reserve(count);
			for (int i = 0; i < count; i++)
			{
				float pos[3] = { lcg.getf(), lcg.getf(), lcg.getf() };
				auto kpos = W.WriteStringKey("localPosition");
				auto vpos = W.WriteVectorT(pos, 3);
				float rot[4] = { lcg.getf(), lcg.getf(), lcg.getf(), lcg.getf() };
				auto krot = W.WriteStringKey("localRotation");
				auto vrot = W.WriteVectorT(rot, 4);
				float scale[4] = { 1, 1, 1 };
				auto kscale = W.WriteStringKey("localScale");
				auto vscale = W.WriteVectorT(scale, 3);
				auto kparent = W.WriteStringKey("parent");
				auto vparent = W.WriteS32(-1);

				StringMapEntry entries[4] =
				{
					{ kpos, vpos },
					{ krot, vrot },
					{ kscale, vscale },
					{ kparent, vparent },
				};
				auto node = W.WriteStringMap(entries, 4);
				vrnodes.push_back(node);
			}
			auto vnodes = W.WriteArray(vrnodes.data(), vrnodes.size());
			W.SetRoot(vnodes);
		}
	}
	printf("size=%u\n", unsigned(W.GetSize()));
	{
		Benchmark B("iter-nodes");//, 100000, 10);
		while (B.Iterate())
		{
			RDR rdr;
			rdr.Init(W.GetData(), W.GetSize());
			NULLValueIterator it;
			rdr.GetRoot().Iterate(it);
		}
	}
	{
		Benchmark B("read-nodes");//, 100000, 10);
		while (B.Iterate())
		{
			RDR rdr;
			if (rdr.Init(W.GetData(), W.GetSize()))
			{
				if (auto arr = rdr.GetRoot().TryGetArray())
				{
					for (u32 i = 0; i < arr.GetSize(); i++)
					{
						if (auto obj = arr[i].TryGetStringMap())
						{
							if (auto vpos = obj.FindValueByKey("localPosition"))
							{
								if (auto vv = vpos.TryGetVector<float>(3))
								{
									float v[3];
									vv.CopyTo(v, 3);
									DoNotOpt(v);
								}
							}
							if (auto vrot = obj.FindValueByKey("localRotation"))
							{
								if (auto vv = vrot.TryGetVector<float>(4))
								{
									float v[4];
									vv.CopyTo(v, 4);
									DoNotOpt(v);
								}
							}
							if (auto vscale = obj.FindValueByKey("localScale"))
							{
								if (auto vv = vscale.TryGetVector<float>(3))
								{
									float v[3];
									vv.CopyTo(v, 3);
									DoNotOpt(v);
								}
							}
							if (auto vparent = obj.FindValueByKey("parent"))
							{
								if (vparent.IsNumber())
								{
									auto v = vparent.CastToNumber<s32>();
									DoNotOpt(v);
								}
							}
						}
					}
				}
			}
		}
	}
	SaveBuffer("nodes" DATO_STRINGIFY(CONFIG) ".gen.dato", W);
	{
		FILE* fp = fopen("nodes" DATO_STRINGIFY(CONFIG) ".gen.dump.txt", "w");
		RDR rdr;
		rdr.Init(W.GetData(), W.GetSize());
		FILEValueDumperIterator fvdi(fp);
		rdr.GetRoot().Iterate(fvdi);
		fclose(fp);
	}
}


int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		puts("argument required");
		return 1;
	}
	char* cmd = argv[1];
	if (streq(cmd, "gen-nodes")) gen_nodes(argc, argv);
	else
	{
		puts("unknown command");
		return 1;
	}
	return 0;
}
