#line 2 "buildtest-reader.cpp"
using namespace dato;

template <class T> void TypedArrayUser(const T& ca)
{
	auto& a = const_cast<T&>(ca);
	a.GetSize();
	a[1];
	for (const auto& it : a) (void)it;
}

template <class T> void VectorUser(const T& ca)
{
	if (ca) (void) ca;
	auto& a = const_cast<T&>(ca);
	a.GetElementCount();
	a[1];
	a.CopyTo(nullptr, 0);
	a.CopyTo_SkipChecks(nullptr, 0);
}

template <class T> void VectorArrayUser(const T& ca)
{
	if (ca) (void) ca;
	auto& a = const_cast<T&>(ca);
	a.GetElementCount();
	a.GetSize();
	a[1];
	for (const auto& it : ca)
	{
		it[1];
	}
	a.CopyTo(nullptr, 0);
	a.CopyTo_SkipChecks(nullptr, 0);
}

template <class Config> void TestReader()
{
	Reader<Config> r;
	r.Init(nullptr, 0);
	auto dyn = r.GetRoot();
#ifdef CANDUMP
	{
		FILEValueDumperIterator it;
		dyn.Iterate(it);
	}
#endif
	{
		if (dyn) (void) dyn;
		dyn.GetType();
		dyn.IsNull();
		dyn.GetSubtype();
		dyn.GetElementCount();
		dyn.IsVector(0, 0);
		dyn.template IsVectorT<s8>(0);
		dyn.template IsVectorT<u8>(0);
		dyn.template IsVectorT<s16>(0);
		dyn.template IsVectorT<u16>(0);
		dyn.template IsVectorT<s32>(0);
		dyn.template IsVectorT<u32>(0);
		dyn.template IsVectorT<s64>(0);
		dyn.template IsVectorT<u64>(0);
		dyn.template IsVectorT<f32>(0);
		dyn.template IsVectorT<f64>(0);
		dyn.IsVectorArray(0, 0);
		dyn.template IsVectorArrayT<s8>(0);
		dyn.template IsVectorArrayT<u8>(0);
		dyn.template IsVectorArrayT<s16>(0);
		dyn.template IsVectorArrayT<u16>(0);
		dyn.template IsVectorArrayT<s32>(0);
		dyn.template IsVectorArrayT<u32>(0);
		dyn.template IsVectorArrayT<s64>(0);
		dyn.template IsVectorArrayT<u64>(0);
		dyn.template IsVectorArrayT<f32>(0);
		dyn.template IsVectorArrayT<f64>(0);
	}
	{
		dyn.AsBool();
		dyn.AsS32();
		dyn.AsU32();
		dyn.AsF32();
		dyn.AsS64();
		dyn.AsU64();
		dyn.AsF64();
	}
	{
		auto sm = dyn.AsStringMap();
		if (sm) (void) sm;
		for (const auto& it : sm)
		{
			it.GetKeyCStr();
			it.GetKeyLength();
			it.GetValue();
		}
		sm[1];
		sm.GetKeyCStr(0);
		sm.GetKeyLength(0);
		sm.FindValueByKey("");
		sm.FindValueByKey(nullptr, 0);
	}
	{
		auto im = dyn.AsIntMap();
		if (im) (void) im;
		for (const auto& it : im)
		{
			it.GetKey();
			it.GetValue();
		}
		im[1];
		im.GetKey(0);
		im.FindValueByKey(0);
	}
	{
		auto arr = dyn.AsArray();
		if (arr) (void) arr;
		arr.GetSize();
		for (const auto& it : arr) (void)it;
		arr.TryGetValueByIndex(0);
		arr.GetValueByIndex(0);
	}
	{
		TypedArrayUser(dyn.AsString8());
		TypedArrayUser(dyn.AsString16());
		TypedArrayUser(dyn.AsString32());
		TypedArrayUser(dyn.AsByteArray());
	}
	{
		VectorUser(dyn.template AsVector<s8>());
		VectorUser(dyn.template AsVector<u8>());
		VectorUser(dyn.template AsVector<s16>());
		VectorUser(dyn.template AsVector<u16>());
		VectorUser(dyn.template AsVector<s32>());
		VectorUser(dyn.template AsVector<u32>());
		VectorUser(dyn.template AsVector<s64>());
		VectorUser(dyn.template AsVector<u64>());
		VectorUser(dyn.template AsVector<f32>());
		VectorUser(dyn.template AsVector<f64>());
	}
	{
		VectorArrayUser(dyn.template AsVectorArray<s8>());
		VectorArrayUser(dyn.template AsVectorArray<u8>());
		VectorArrayUser(dyn.template AsVectorArray<s16>());
		VectorArrayUser(dyn.template AsVectorArray<u16>());
		VectorArrayUser(dyn.template AsVectorArray<s32>());
		VectorArrayUser(dyn.template AsVectorArray<u32>());
		VectorArrayUser(dyn.template AsVectorArray<s64>());
		VectorArrayUser(dyn.template AsVectorArray<u64>());
		VectorArrayUser(dyn.template AsVectorArray<f32>());
		VectorArrayUser(dyn.template AsVectorArray<f64>());
	}
	{
		dyn.TryGetStringMap();
		dyn.TryGetIntMap();
		dyn.TryGetArray();
		dyn.TryGetString8();
		dyn.TryGetString16();
		dyn.TryGetString32();
		dyn.TryGetByteArray();
	}
	{
		dyn.template TryGetVector<s8>();
		dyn.template TryGetVector<u8>();
		dyn.template TryGetVector<s16>();
		dyn.template TryGetVector<u16>();
		dyn.template TryGetVector<s32>();
		dyn.template TryGetVector<u32>();
		dyn.template TryGetVector<s64>();
		dyn.template TryGetVector<u64>();
		dyn.template TryGetVector<f32>();
		dyn.template TryGetVector<f64>();
	}
	{
		dyn.template TryGetVectorArray<s8>();
		dyn.template TryGetVectorArray<u8>();
		dyn.template TryGetVectorArray<s16>();
		dyn.template TryGetVectorArray<u16>();
		dyn.template TryGetVectorArray<s32>();
		dyn.template TryGetVectorArray<u32>();
		dyn.template TryGetVectorArray<s64>();
		dyn.template TryGetVectorArray<u64>();
		dyn.template TryGetVectorArray<f32>();
		dyn.template TryGetVectorArray<f64>();
	}
	{
		dyn.template CastToNumber<s32>();
		dyn.template CastToNumber<u32>();
		dyn.template CastToNumber<f32>();
		dyn.template CastToNumber<s64>();
		dyn.template CastToNumber<u64>();
		dyn.template CastToNumber<f64>();
		dyn.CastToBool();
	}
}

int main()
{
	TestReader<ReaderConfig0>();
	TestReader<ReaderConfig1>();
	TestReader<ReaderConfig2>();
	TestReader<ReaderConfig3>();
	TestReader<ReaderConfig4>();
	TestReader<ReaderAdaptiveConfig>();
}
