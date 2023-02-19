
// DATO file format data dumping extension for the reader library - v1.0
// See the end of this file for license information

#pragma once
#include "dato_reader.hpp"

#include <stdio.h>


namespace dato {

inline const char* SubtypeToString(u8 subtype)
{
	switch (subtype)
	{
	case SUBTYPE_S8: return "s8";
	case SUBTYPE_U8: return "u8";
	case SUBTYPE_S16: return "s16";
	case SUBTYPE_U16: return "u16";
	case SUBTYPE_S32: return "s32";
	case SUBTYPE_U32: return "u32";
	case SUBTYPE_S64: return "s64";
	case SUBTYPE_U64: return "u64";
	case SUBTYPE_F32: return "f32";
	case SUBTYPE_F64: return "f64";
	default: return "?";
	}
}

struct IValueDumperIterator : IValueIterator
{
	// configuration
	const char* indentText = "  ";
	// output interface
	virtual void PrintText(const char* text, u32 len) = 0;

	// state
	int _indent = 0;
	// helpers
	void _WriteIndent()
	{
		const char* ite = indentText;
		while (*ite)
			ite++;
		u32 len = u32(ite - indentText);
		for (int i = 0; i < _indent; i++)
			PrintText(indentText, len);
	}

	// IValueIterator implementation
	void BeginMap(u8 type, u32 size) override
	{
		char bfr[32];
		PrintText(bfr, snprintf(
			bfr,
			32,
			"map(%s) [%u]\n",
			type == TYPE_StringMap ? "str" : "int",
			unsigned(size)));
		_WriteIndent();
		PrintText("{\n", 2);
		_indent++;
	}
	void EndMap(u8 type) override
	{
		(void)type;
		_indent--;
		_WriteIndent();
		PrintText("}", 1);
	}
	void BeginStringKey(const char* key, u32 length) override
	{
		_WriteIndent();
		PrintText("\"", 1);
		PrintText(key, length);
		PrintText("\" = ", 4);
	}
	void EndStringKey() override
	{
		PrintText("\n", 1);
	}
	void BeginIntKey(u32 key) override
	{
		_WriteIndent();
		char bfr[32];
		PrintText(bfr, snprintf(bfr, 32, "%08X = ", unsigned(key)));
	}
	void EndIntKey() override
	{
		PrintText("\n", 1);
	}

	void BeginArray(u32 size) override
	{
		char bfr[32];
		PrintText(bfr, snprintf(
			bfr,
			32,
			"array [%u]\n",
			unsigned(size)));
		_WriteIndent();
		PrintText("{\n", 2);
		_indent++;
	}
	void EndArray() override
	{
		_indent--;
		_WriteIndent();
		PrintText("}", 1);
	}
	void BeginArrayIndex(u32 i) override
	{
		_WriteIndent();
		char bfr[32];
		PrintText(bfr, snprintf(bfr, 32, "%u = ", unsigned(i)));
	}
	void EndArrayIndex() override
	{
		PrintText("\n", 1);
	}

	void OnValueNull() override
	{
		PrintText("null", 4);
	}
	void OnValueBool(bool value) override
	{
		if (value)
			PrintText("true", 4);
		else
			PrintText("false", 5);
	}
	void OnValueS32(s32 value) override
	{
		char bfr[32];
		PrintText(bfr, snprintf(bfr, 32, "s32:%d", int(value)));
	}
	void OnValueU32(u32 value) override
	{
		char bfr[32];
		PrintText(bfr, snprintf(bfr, 32, "u32:%u", unsigned(value)));
	}
	void OnValueF32(f32 value) override
	{
		char bfr[32];
		PrintText(bfr, snprintf(bfr, 32, "f32:%g", value));
	}
	void OnValueS64(s64 value) override
	{
		char bfr[32];
		PrintText(bfr, snprintf(bfr, 32, "s64:%lld", (long long) value));
	}
	void OnValueU64(u64 value) override
	{
		char bfr[32];
		PrintText(bfr, snprintf(bfr, 32, "u64:%llu", (unsigned long long) value));
	}
	void OnValueF64(f64 value) override
	{
		char bfr[32];
		PrintText(bfr, snprintf(bfr, 32, "f64:%g", value));
	}
	// String[8|16|32]
	template <class T> void _PrintString(const T* data, u32 length)
	{
		for (u32 i = 0; i < length; i++)
		{
			T v = ReadT<T>(&data[i]);
			if (v == '\\')
			{
				PrintText("\\\\", 2);
			}
			else if (v >= 32 && v < 127)
			{
				char c = char(v);
				PrintText(&c, 1);
			}
			else
			{
				char bfr[32];
				PrintText(bfr, snprintf(bfr, 32, "\\x%02X", unsigned(v)));
			}
		}
	}
	void OnValueString(const char* data, u32 length) override
	{
		PrintText("str8:\"", 6);
		_PrintString((const u8*) data, length);
		PrintText("\"", 1);
	}
	void OnValueString(const u16* data, u32 length) override
	{
		PrintText("str16:\"", 7);
		_PrintString((const u16*) data, length);
		PrintText("\"", 1);
	}
	void OnValueString(const u32* data, u32 length) override
	{
		PrintText("str32:\"", 7);
		_PrintString((const u32*) data, length);
		PrintText("\"", 1);
	}
	void OnValueByteArray(const void* data, u32 length) override
	{
		char bfr[32];
		PrintText(bfr, snprintf(bfr, 32, "bytearray [%u]:", unsigned(length)));
		for (u32 i = 0; i < length; i++)
		{
			u8 v = ((const u8*) data)[i];
			char bytechars[2] =
			{
				"0123456789ABCDEF"[v >> 4],
				"0123456789ABCDEF"[v & 15],
			};
			PrintText(bytechars, 2);
		}
	}
	void _PrintSubvalues(u8 subtype, u32 count, const void* data)
	{
		for (u32 i = 0; i < count; i++)
		{
			if (i)
				PrintText(";", 1);

			char bfr[32];
			u32 len = 0;
			switch (subtype)
			{
			case SUBTYPE_S8: len = snprintf(bfr, 32, "%d", int(((const s8*) data)[i])); break;
			case SUBTYPE_U8: len = snprintf(bfr, 32, "%u", unsigned(((const u8*) data)[i])); break;
			case SUBTYPE_S16: len = snprintf(bfr, 32, "%d", int(((const s16*) data)[i])); break;
			case SUBTYPE_U16: len = snprintf(bfr, 32, "%u", unsigned(((const u16*) data)[i])); break;
			case SUBTYPE_S32: len = snprintf(bfr, 32, "%d", int(((const s32*) data)[i])); break;
			case SUBTYPE_U32: len = snprintf(bfr, 32, "%u", unsigned(((const u32*) data)[i])); break;
			case SUBTYPE_S64:
				len = snprintf(bfr, 32, "%lld", (long long) (((const s64*) data)[i]));
				break;
			case SUBTYPE_U64:
				len = snprintf(bfr, 32, "%llu", (unsigned long long) (((const u64*) data)[i]));
				break;
			case SUBTYPE_F32: len = snprintf(bfr, 32, "%g", ((const float*) data)[i]); break;
			case SUBTYPE_F64: len = snprintf(bfr, 32, "%g", ((const double*) data)[i]); break;
			default: bfr[len++] = '?'; bfr[len] = 0; break;
			}
			PrintText(bfr, len);
		}
	}
	void OnValueVector(u8 subtype, u8 elemCount, const void* data) override
	{
		char bfr[32];
		PrintText(bfr, snprintf(
			bfr,
			32,
			"vector(%s, %u):[",
			SubtypeToString(subtype),
			unsigned(elemCount)));

		_PrintSubvalues(subtype, elemCount, data);

		PrintText("]", 1);
	}
	void OnValueVectorArray(u8 subtype, u8 elemCount, const void* data, u32 length) override
	{
		char bfr[32];
		PrintText(bfr, snprintf(
			bfr,
			32,
			"vectorarray(%s, %u) [%u]:[",
			SubtypeToString(subtype),
			unsigned(elemCount),
			unsigned(length)));

		for (u32 i = 0; i < length; i++)
		{
			if (i)
				PrintText(" | ", 3);
			_PrintSubvalues(subtype, elemCount, data);
		}

		PrintText("]", 1);
	}
	void OnUnknownValue(u8 type, u32 embedded, const char* buffer, u32 length) override
	{
		(void)buffer;
		(void)length;
		char bfr[48];
		u32 len = snprintf(bfr, 48, "unknown (type=%u, data=%u)", unsigned(type), unsigned(embedded));
		PrintText(bfr, len);
	}
};

struct FILEValueDumperIterator : IValueDumperIterator
{
	FILE* file = nullptr;

	FILEValueDumperIterator() {}
	FILEValueDumperIterator(FILE* f) : file(f) {}
	FILEValueDumperIterator(FILE* f, const char* indent) : file(f) { indentText = indent; }

	void PrintText(const char* text, u32 len) override
	{
		fwrite(text, len, 1, file);
	}
};

} // dato

/*
This software is available under 2 licenses:
-------------------------------------------------------------------------------
OPTION 1: MIT License

Copyright (c) 2023 Arvīds Kokins

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the “Software”), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
-------------------------------------------------------------------------------
OPTION 2: Unlicense

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
*/
