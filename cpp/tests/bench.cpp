
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

int main()
{
	Overhead();
	IntSortSpeed();
}
