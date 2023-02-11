#line 2 "buildtest-writer.cpp"
using namespace dato;

template <class Config> void TestWriter()
{
	Writer<Config> wr;
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
	wr.template WriteVectorT<s8>(nullptr, 3);
	wr.template WriteVectorT<u8>(nullptr, 3);
	wr.template WriteVectorT<s16>(nullptr, 3);
	wr.template WriteVectorT<u16>(nullptr, 3);
	wr.template WriteVectorT<s32>(nullptr, 3);
	wr.template WriteVectorT<u32>(nullptr, 3);
	wr.template WriteVectorT<s64>(nullptr, 3);
	wr.template WriteVectorT<u64>(nullptr, 3);
	wr.template WriteVectorT<f32>(nullptr, 3);
	wr.template WriteVectorT<f64>(nullptr, 3);
	wr.template WriteVectorArrayT<s8>(nullptr, 3, 0);
	wr.template WriteVectorArrayT<u8>(nullptr, 3, 0);
	wr.template WriteVectorArrayT<s16>(nullptr, 3, 0);
	wr.template WriteVectorArrayT<u16>(nullptr, 3, 0);
	wr.template WriteVectorArrayT<s32>(nullptr, 3, 0);
	wr.template WriteVectorArrayT<u32>(nullptr, 3, 0);
	wr.template WriteVectorArrayT<s64>(nullptr, 3, 0);
	wr.template WriteVectorArrayT<u64>(nullptr, 3, 0);
	wr.template WriteVectorArrayT<f32>(nullptr, 3, 0);
	wr.template WriteVectorArrayT<f64>(nullptr, 3, 0);
}

int main()
{
	TestWriter<WriterConfig0>();
	TestWriter<WriterConfig1>();
	TestWriter<WriterConfig2>();
	TestWriter<WriterConfig3>();
	TestWriter<WriterConfig4>();
}
