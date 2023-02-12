
#include <stdio.h>

#ifdef __clang__
template <class T> inline __attribute__((always_inline)) void DoNotOpt(T& val)
{
	asm volatile("" : "+r,m"(val) : : "memory");
}
#else
void SINK(void*);
#define DoNotOpt(x) SINK((void*)&(x))
#endif

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
	Benchmark(const char* name_, int maxN = 100000, float maxSec = 0.1f)
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
