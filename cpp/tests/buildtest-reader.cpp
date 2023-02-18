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

void TestReader()
{
	Reader r;
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
		dyn.IsVectorT<s8>(0);
		dyn.IsVectorT<u8>(0);
		dyn.IsVectorT<s16>(0);
		dyn.IsVectorT<u16>(0);
		dyn.IsVectorT<s32>(0);
		dyn.IsVectorT<u32>(0);
		dyn.IsVectorT<s64>(0);
		dyn.IsVectorT<u64>(0);
		dyn.IsVectorT<f32>(0);
		dyn.IsVectorT<f64>(0);
		dyn.IsVectorArray(0, 0);
		dyn.IsVectorArrayT<s8>(0);
		dyn.IsVectorArrayT<u8>(0);
		dyn.IsVectorArrayT<s16>(0);
		dyn.IsVectorArrayT<u16>(0);
		dyn.IsVectorArrayT<s32>(0);
		dyn.IsVectorArrayT<u32>(0);
		dyn.IsVectorArrayT<s64>(0);
		dyn.IsVectorArrayT<u64>(0);
		dyn.IsVectorArrayT<f32>(0);
		dyn.IsVectorArrayT<f64>(0);
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
		VectorUser(dyn.AsVector<s8>());
		VectorUser(dyn.AsVector<u8>());
		VectorUser(dyn.AsVector<s16>());
		VectorUser(dyn.AsVector<u16>());
		VectorUser(dyn.AsVector<s32>());
		VectorUser(dyn.AsVector<u32>());
		VectorUser(dyn.AsVector<s64>());
		VectorUser(dyn.AsVector<u64>());
		VectorUser(dyn.AsVector<f32>());
		VectorUser(dyn.AsVector<f64>());
	}
	{
		VectorUser(dyn.AsVector<s8>(1));
		VectorUser(dyn.AsVector<u8>(1));
		VectorUser(dyn.AsVector<s16>(1));
		VectorUser(dyn.AsVector<u16>(1));
		VectorUser(dyn.AsVector<s32>(1));
		VectorUser(dyn.AsVector<u32>(1));
		VectorUser(dyn.AsVector<s64>(1));
		VectorUser(dyn.AsVector<u64>(1));
		VectorUser(dyn.AsVector<f32>(1));
		VectorUser(dyn.AsVector<f64>(1));
	}
	{
		VectorArrayUser(dyn.AsVectorArray<s8>());
		VectorArrayUser(dyn.AsVectorArray<u8>());
		VectorArrayUser(dyn.AsVectorArray<s16>());
		VectorArrayUser(dyn.AsVectorArray<u16>());
		VectorArrayUser(dyn.AsVectorArray<s32>());
		VectorArrayUser(dyn.AsVectorArray<u32>());
		VectorArrayUser(dyn.AsVectorArray<s64>());
		VectorArrayUser(dyn.AsVectorArray<u64>());
		VectorArrayUser(dyn.AsVectorArray<f32>());
		VectorArrayUser(dyn.AsVectorArray<f64>());
	}
	{
		VectorArrayUser(dyn.AsVectorArray<s8>(1));
		VectorArrayUser(dyn.AsVectorArray<u8>(1));
		VectorArrayUser(dyn.AsVectorArray<s16>(1));
		VectorArrayUser(dyn.AsVectorArray<u16>(1));
		VectorArrayUser(dyn.AsVectorArray<s32>(1));
		VectorArrayUser(dyn.AsVectorArray<u32>(1));
		VectorArrayUser(dyn.AsVectorArray<s64>(1));
		VectorArrayUser(dyn.AsVectorArray<u64>(1));
		VectorArrayUser(dyn.AsVectorArray<f32>(1));
		VectorArrayUser(dyn.AsVectorArray<f64>(1));
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
		dyn.TryGetVector<s8>();
		dyn.TryGetVector<u8>();
		dyn.TryGetVector<s16>();
		dyn.TryGetVector<u16>();
		dyn.TryGetVector<s32>();
		dyn.TryGetVector<u32>();
		dyn.TryGetVector<s64>();
		dyn.TryGetVector<u64>();
		dyn.TryGetVector<f32>();
		dyn.TryGetVector<f64>();
	}
	{
		dyn.TryGetVector<s8>(1);
		dyn.TryGetVector<u8>(1);
		dyn.TryGetVector<s16>(1);
		dyn.TryGetVector<u16>(1);
		dyn.TryGetVector<s32>(1);
		dyn.TryGetVector<u32>(1);
		dyn.TryGetVector<s64>(1);
		dyn.TryGetVector<u64>(1);
		dyn.TryGetVector<f32>(1);
		dyn.TryGetVector<f64>(1);
	}
	{
		dyn.TryGetVectorArray<s8>();
		dyn.TryGetVectorArray<u8>();
		dyn.TryGetVectorArray<s16>();
		dyn.TryGetVectorArray<u16>();
		dyn.TryGetVectorArray<s32>();
		dyn.TryGetVectorArray<u32>();
		dyn.TryGetVectorArray<s64>();
		dyn.TryGetVectorArray<u64>();
		dyn.TryGetVectorArray<f32>();
		dyn.TryGetVectorArray<f64>();
	}
	{
		dyn.TryGetVectorArray<s8>(1);
		dyn.TryGetVectorArray<u8>(1);
		dyn.TryGetVectorArray<s16>(1);
		dyn.TryGetVectorArray<u16>(1);
		dyn.TryGetVectorArray<s32>(1);
		dyn.TryGetVectorArray<u32>(1);
		dyn.TryGetVectorArray<s64>(1);
		dyn.TryGetVectorArray<u64>(1);
		dyn.TryGetVectorArray<f32>(1);
		dyn.TryGetVectorArray<f64>(1);
	}
	{
		dyn.CastToNumber<s32>();
		dyn.CastToNumber<u32>();
		dyn.CastToNumber<f32>();
		dyn.CastToNumber<s64>();
		dyn.CastToNumber<u64>();
		dyn.CastToNumber<f64>();
		dyn.CastToBool();
	}
}

int main()
{
	TestReader();
}
