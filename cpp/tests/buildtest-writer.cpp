#line 2 "buildtest-writer.cpp"
using namespace dato;

void TestWriter()
{
	Writer wr;
	wr.SetRoot(wr.WriteNull());
	wr.WriteBool(false);
	wr.WriteBool(true);
	wr.WriteS32(-123456789);
	wr.WriteU32(123456789);
	wr.WriteF32(0.123456f);
	wr.WriteS64(-1234567890987654321);
	wr.WriteU64(1234567890987654321);
	wr.WriteF64(0.123456789);
	wr.WriteArray(nullptr, 0);
	wr.WriteStringMap(nullptr, 0);
	wr.WriteIntMap(nullptr, 0);
	wr.WriteString8("");
	wr.WriteString16(u"");
	wr.WriteString32(U"");
	wr.WriteByteArray(nullptr, 0);
	wr.WriteVectorT<s8>(nullptr, 3);
	wr.WriteVectorT<u8>(nullptr, 3);
	wr.WriteVectorT<s16>(nullptr, 3);
	wr.WriteVectorT<u16>(nullptr, 3);
	wr.WriteVectorT<s32>(nullptr, 3);
	wr.WriteVectorT<u32>(nullptr, 3);
	wr.WriteVectorT<s64>(nullptr, 3);
	wr.WriteVectorT<u64>(nullptr, 3);
	wr.WriteVectorT<f32>(nullptr, 3);
	wr.WriteVectorT<f64>(nullptr, 3);
	wr.WriteVectorArrayT<s8>(nullptr, 3, 0);
	wr.WriteVectorArrayT<u8>(nullptr, 3, 0);
	wr.WriteVectorArrayT<s16>(nullptr, 3, 0);
	wr.WriteVectorArrayT<u16>(nullptr, 3, 0);
	wr.WriteVectorArrayT<s32>(nullptr, 3, 0);
	wr.WriteVectorArrayT<u32>(nullptr, 3, 0);
	wr.WriteVectorArrayT<s64>(nullptr, 3, 0);
	wr.WriteVectorArrayT<u64>(nullptr, 3, 0);
	wr.WriteVectorArrayT<f32>(nullptr, 3, 0);
	wr.WriteVectorArrayT<f64>(nullptr, 3, 0);
}

int main()
{
	TestWriter();
}
