
#include "../dato_reader.hpp"


int main()
{
	dato::UniversalBufferReader r;
	auto root = r.GetRoot();
	root.FindValueByStringKey("what");
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
