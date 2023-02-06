
#include "../dato_writer.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <algorithm>


void SINK(void*);
#define DoNotOpt(x) SINK((void*)&(x))

using Timestamp = long long;

#if _WIN32
using BOOL = int;
struct LARGE_INTEGER { long long QuadPart; };
extern "C" __declspec(dllimport) BOOL __stdcall QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount);
extern "C" __declspec(dllimport) BOOL __stdcall QueryPerformanceFrequency(LARGE_INTEGER* lpFrequency);

DATO_FORCEINLINE Timestamp GetTime()
{
	LARGE_INTEGER i;
	QueryPerformanceCounter(&i);
	return i.QuadPart;
}
DATO_FORCEINLINE Timestamp GetFrequency()
{
	LARGE_INTEGER i;
	QueryPerformanceFrequency(&i);
	return i.QuadPart;
}
#endif

double GetMs(Timestamp t, Timestamp f)
{
	return double(t) * 1000 / double(f);
}

struct Benchmark
{
	const char* name;
	Timestamp freq, start, end, total = 0, curStart, timeLimit;
	bool started = false;
	int n = 0;
	int remaining;
	Benchmark(const char* name_, float maxSec = 0.1f, int maxN = 100000)
		: name(name_), remaining(maxN)
	{
		freq = GetFrequency();
		timeLimit = Timestamp(maxSec * freq);
	}
	~Benchmark()
	{
		printf("%s: %d iterations, %.0f ms (%.0f ms work, %.3f us/it)\n",
			name,
			n,
			GetMs(end - start, freq),
			GetMs(total, freq),
			GetMs(total, freq) * 1000 / n);
	}
	bool Iterate()
	{
		if (!started)
		{
			started = true;
			start = end = curStart = GetTime();
			return true;
		}
		else
		{
			end = GetTime();
			total += end - curStart;
			if (end - start > timeLimit)
				return false;
			n++;
			if (--remaining <= 0)
				return false;
			curStart = GetTime();
			return true;
		}
	}
	void PrepDone()
	{
		curStart = GetTime();
	}
};

void Overhead()
{
	Benchmark B("overhead");
	while (B.Iterate()) {}
}

void IntSortSpeed()
{
	using namespace dato;
	TempMem tm;
	tm.GetData<IntMapEntry>(100);
	IntMapEntry entries[100];
	int FIRST = 56;
	int LAST = 64;
	for (int N = FIRST; N < LAST; N++)
	{
		char buf[32];
		sprintf(buf, "insertion sort (%d)", N);
		Benchmark B(buf);
		while (B.Iterate())
		{
			for (int i = 0; i < N; i++)
				entries[i].key = rand();
			B.PrepDone();
			SortEntriesByKeyInt_Insertion(tm, entries, N);
			DoNotOpt(entries);
		}
	}
	for (int N = FIRST; N < LAST; N++)
	{
		char buf[32];
		sprintf(buf, "radix sort (%d)", N);
		Benchmark B(buf);
		while (B.Iterate())
		{
			for (int i = 0; i < N; i++)
				entries[i].key = rand();
			B.PrepDone();
			SortEntriesByKeyInt_Radix(tm, entries, N);
			DoNotOpt(entries);
		}
	}
	for (int N = FIRST; N < LAST; N++)
	{
		char buf[32];
		sprintf(buf, "std::sort (%d)", N);
		Benchmark B(buf);
		while (B.Iterate())
		{
			for (int i = 0; i < N; i++)
				entries[i].key = rand();
			B.PrepDone();
			std::sort(entries, entries + N, [](const IntMapEntry& a, const IntMapEntry& b)
			{
				return a.key < b.key;
			});
			DoNotOpt(entries);
		}
	}
}

static char sortStringBuf[1024 * 16];

static void SortStringMapEntries_STD(dato::StringMapEntry* entries, dato::u32 count)
{
	using namespace dato;
	std::sort(
		entries,
		entries + count,
		[](const StringMapEntry& a, const StringMapEntry& b)
	{
		u32 minSize = a.key.dataLen < b.key.dataLen ? a.key.dataLen : b.key.dataLen;
		const char* ka = sortStringBuf + a.key.dataPos;
		const char* kb = sortStringBuf + b.key.dataPos;
		if (int diff = memcmp(ka, kb, minSize))
			return diff < 0;
		return a.key.dataLen < b.key.dataLen;
	});
}

static const char IDCHARS[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";

static void GenSortingData_RandomChars(
	dato::StringMapEntry* entries,
	dato::u32 numEntries,
	dato::u32 nchars,
	dato::u32 npfx)
{
	using namespace dato;

	char* p = sortStringBuf;
	for (u32 i = 0; i < numEntries; i++)
	{
		auto& e = entries[i];
		e.key.dataPos = p - sortStringBuf;
		e.key.pos = p - sortStringBuf + 1;
		e.key.dataLen = nchars;
		for (u32 j = 0; j < npfx; j++)
			*p++ = '^'; // between uppercase and lowercase characters
		for (u32 j = 0; j < nchars; j++)
			*p++ = IDCHARS[rand() % (sizeof(IDCHARS) - 1)];
	}
}

static unsigned GenSortingData_FromZSSL(
	dato::StringMapEntry* entries,
	const char* zssl)
{
	using namespace dato;

	char* p = sortStringBuf;
	unsigned i = 0;
	for (const char* w = zssl; *w; w += strlen(w) + 1)
	{
		auto& e = entries[i++];
		e.key.dataPos = p - sortStringBuf;
		e.key.pos = p - sortStringBuf + 1;
		u32 len = u32(strlen(w));
		e.key.dataLen = len;
		memcpy(p, w, len + 1);
		p += len + 1;
	}
	return i;
}

void StringSortSpeed_RandomChars()
{
	using namespace dato;
	StringMapEntry entries[100];
	int FIRST = 3;
	int LAST = 12;
	// random chars
	for (int N = FIRST; N <= LAST; N++)
	{
		char buf[32];
		sprintf(buf, "q3str sort (random/%d)", N);
		Benchmark B(buf);
		while (B.Iterate())
		{
			GenSortingData_RandomChars(entries, N, 10, 0);
			B.PrepDone();
			SortEntriesByKeyString(sortStringBuf, entries, N);
			DoNotOpt(entries);
		}
	}
	for (int N = FIRST; N <= LAST; N++)
	{
		char buf[32];
		sprintf(buf, "std::sort (random/%d)", N);
		Benchmark B(buf);
		while (B.Iterate())
		{
			GenSortingData_RandomChars(entries, N, 10, 0);
			B.PrepDone();
			SortStringMapEntries_STD(entries, N);
			DoNotOpt(entries);
		}
	}
	// prefixed + random chars
	for (int N = FIRST; N <= LAST; N++)
	{
		char buf[32];
		sprintf(buf, "q3str sort (pfx+random/%d)", N);
		Benchmark B(buf);
		while (B.Iterate())
		{
			GenSortingData_RandomChars(entries, N, 10, 10);
			B.PrepDone();
			SortEntriesByKeyString(sortStringBuf, entries, N);
			DoNotOpt(entries);
		}
	}
	for (int N = FIRST; N <= LAST; N++)
	{
		char buf[32];
		sprintf(buf, "std::sort (pfx+random/%d)", N);
		Benchmark B(buf);
		while (B.Iterate())
		{
			GenSortingData_RandomChars(entries, N, 10, 10);
			B.PrepDone();
			SortStringMapEntries_STD(entries, N);
			DoNotOpt(entries);
		}
	}
}

void StringSortSpeed_SpecificSets()
{
	using namespace dato;
	StringMapEntry entries[100];
	// various entry sets
	const char* entrySets[] =
	{
		// font character example
		"offX\0offY\0sizeX\0sizeY\0advX\0advY\0page\0",
		// generic node example
		"parent\0name\0position\0rotation\0scale\0",
		// GLTF
		"bufferView\0byteOffset\0componentType\0normalized\0count\0type\0"
		/**/"max\0min\0sparse\0name\0extensions\0extras\0", // accessor
		"buffer\0byteOffset\0byteLength\0byteStride\0target\0name\0extensions\0extras\0", // buffer view
		"extensionsUsed\0extensionsRequired\0accessors\0animations\0asset\0"
		/**/"buffers\0bufferViews\0cameras\0images\0materials\0meshes\0nodes\0"
		/**/"samplers\0scene\0scenes\0skins\0textures\0extensions\0extras\0", // root
		"name\0extensions\0extras\0pbrMetallicRoughness\0normalTexture\0occlusionTexture\0"
		/**/"emissiveTexture\0emissiveFactor\0alphaMode\0alphaCutoff\0doubleSided\0", // material
	};
	for (int set = 0; set < sizeof(entrySets) / sizeof(entrySets[0]); set++)
	{
		char buf[32];
		sprintf(buf, "q3str sort (set %d)", set);
		{
			Benchmark B(buf);
			while (B.Iterate())
			{
				u32 N = GenSortingData_FromZSSL(entries, entrySets[set]);
				B.PrepDone();
				SortEntriesByKeyString(sortStringBuf, entries, N);
				DoNotOpt(entries);
			}
		}
		sprintf(buf, "std::sort (set %d)", set);
		{
			Benchmark B(buf);
			while (B.Iterate())
			{
				u32 N = GenSortingData_FromZSSL(entries, entrySets[set]);
				B.PrepDone();
				SortStringMapEntries_STD(entries, N);
				DoNotOpt(entries);
			}
		}
	}
}

int main()
{
	Overhead();
	//IntSortSpeed();
	StringSortSpeed_RandomChars();
	StringSortSpeed_SpecificSets();
}
