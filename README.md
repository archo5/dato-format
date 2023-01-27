# DATO: Directly Accessing a Tree of Objects
## A single-parse binary file format for encoding extensible data

### Why this format?

Most file formats exist in one of two categories:

* Formats that require two or more passes to extract data (JSON, XML);
	* These are inherently slow to parse.
* Formats that require matching schemas between parsing and writing (RIFF, flatbuffers);
	* These need to have prebuilt upgrade paths.

This format combines the following properties:

* **Single-parse** - the data can be accessed directly, without pre-parsing it, for improved parsing speed
	* The data can be optionally aligned to further improve parsing speed on CPUs that don't support unaligned loads natively
* **Traversable** - there are built-in structures for interpreting most data, which allows the following:
	* Changing the format of any object while retaining backwards compatibility
	* Partially inspecting and validating the data without knowing the entire format
* **Customizable** - the exact details of the format can be optimized for the particular application:
	* It is possible to add more data types
	* The encoding of various length/size values can be changed to optimize for size or performance
	* Object keys can be either names (for readability) or 32-bit hashes (for reduced size and parsing speed)
	* Different string types can be used (to match the application's internal string format)
	* The prefix of the file can be changed

### The file format specification (**warning: not finalized at this point - minor details may change**)

```py
FILE =
{
	PREFIX-BYTES
	CONFIG-BYTE
	PROPERTY-BYTE
	ALIGN(4)
	REF(OBJECT)
}

PREFIX-BYTES = "DATO"
# can be overridden

CONFIG-BYTE = 0 | 1 | 2 | 3 | [128;255]
# values: 0-3 refer to standardized configurations, 4-127 are reserved, ..
# .. 128-255 can be used for specifying application-specific configurations
# config 0: file is aligned, all sizes are 32-bit
#	- Optimizes for reading speed at the cost of size while remaining compatible with all data
# config 1: file is unaligned, all sizes are variable-length (8-40 bits)
#	- Optimizes for size at the cost of reading speed while remaining compatible with all data
# config 2: file is aligned, key and object sizes are 8-bit, array sizes are 32-bit
#	- Optimizes for reading speed first, then size, while breaking compatibility with large objects and keys
# config 3: file is unaligned, key and object sizes are 8-bit, array sizes are variable-length (8-40 bits)
#	- Optimizes for size first, then reading speed, while breaking compatibility with large objects and keys

PROPERTY-BYTE = [0 1 R R R R R R]
# values are flags:
# - bit 0 specifies whether integer keys are used (1 = yes)
#	- integer keys do not point to a KEY-STRING reference but instead are themselves identifiers
# - bit 1 specifies whether the keys are sorted (1 = yes)
#	- integer keys are expected to be sorted by value, string keys by content (case-sensitive)
# - bits 2-7 are reserved

ALIGN(N) = 0 ... 0[N-1]
# if alignment is enabled, this contains up to N-1 zeroes, to align the in-file position to [x % N = 0]

REF(T) = uint32
# an offset from the start of the file (before the prefix)
# in this spec, T optionally specifies the type that is expected to be at the end of the reference

OBJECT =
{
	ALIGN(SIZE(OBJECT), 4) # for mixed-value sizes, the alignment must take into account all the values
	size = SIZE(OBJECT) # the number of properties in the object
	keys = KEY[size]
	values = VALUE[size]
	types = TYPE[size]
}

ARRAY =
{
	ALIGN(SIZE(ARRAY), 4) # for mixed-value sizes, the alignment must take into account all the values
	size = SIZE(ARRAY)
	values = VALUE[size]
	types = TYPE[size]
}

TYPED-ARRAY(T, alignment=sizeof(T), null_terminated=False) =
# in this spec, T optionally specifies the array element type
{
	ALIGN(SIZE(TYPED-ARRAY), alignment) # for mixed-value sizes, the alignment must take into account all the values
	size = SIZE(TYPED-ARRAY)
	values = T[size]
	if null_terminated: T(0)
}
# variations of typed array:
BYTE-ARRAY(alignment) = TYPED-ARRAY(u8, alignment)
STRING-8 = BYTE-ARRAY(char8, null_terminated=True)
STRING-16 = BYTE-ARRAY(char16, null_terminated=True)
STRING-32 = BYTE-ARRAY(char32, null_terminated=True)

SIZE(T) = [depends on encoding]
{
	option 1: uint8(n)
	option 2: uint32(n)
	option 3:
	- if n < 255: uint8(n)
	- if n >= 255: uint8(255) uint32(n)
}

KEY = uint32 | REF(KEY-STRING)

KEY-STRING =
{
	ALIGN(SIZE(KEY-STRING)) # for mixed-value sizes, the alignment must take into account all the values
	size = SIZE(KEY-STRING) # the number of characters in the key string (excludes the terminating zero)
	data = char[size]
	char(0)
}

VALUE = int32 | uint32 | float32 | REF(int64 | uint64 | float64 | ARRAY | OBJECT | TYPED-ARRAY | STRING)

TYPE = uint8
- 0: null (value = 0)
# embedded value types:
- 1: bool (value = 1 | 0)
- 2: int32 (value = input)
- 3: uint32 (value = input)
- 4: float32 (value = input)
# external value reference types:
# - one value:
- 5: int64 (value = REF(int64(input)))
- 6: uint64 (value = REF(uint64(input)))
- 7: float64 (value = REF(float64(input)))
# - generic containers:
- 8: array (value = REF(ARRAY))
- 9: object (value = REF(OBJECT))
# - raw arrays (identified by purpose)
# (strings contain an extra 0-termination value not included in their size)
- 10: string, 8-bit characters (value = REF(STRING-8))
- 11: string, 16-bit characters (value = REF(STRING-16))
- 12: string, 32-bit characters (value = REF(STRING-32))
- 13: byte array (value = REF(BYTE-ARRAY))
# - typed arrays (identified by type)
- 14: int8 array (value = REF(TYPED-ARRAY(int8)))
- 15: uint8 array (value = REF(TYPED-ARRAY(uint8)))
- 16: int16 array (value = REF(TYPED-ARRAY(int16)))
- 17: uint16 array (value = REF(TYPED-ARRAY(uint16)))
- 18: int32 array (value = REF(TYPED-ARRAY(int32)))
- 19: uint32 array (value = REF(TYPED-ARRAY(uint32)))
- 20: int64 array (value = REF(TYPED-ARRAY(int64)))
- 21: uint64 array (value = REF(TYPED-ARRAY(uint64)))
- 22: float32 array (value = REF(TYPED-ARRAY(float32)))
- 23: float64 array (value = REF(TYPED-ARRAY(float64)))
- 24-127: reserved # likely to be used for standardizing frequently used common formats to remove the need to incur the length overhead of putting them into generic typed arrays
- 128-255: application-specific
```

### What was considered and rejected for the format?

- **Interleaved object entries**:
	- They required the largest alignment value to be applied for the entire entry (making it 12 bytes big), which effectively added 3 bytes of padding due to the type being 1 byte big initially.
	- For bigger objects, they would make it necessary to pull unused memory into cache before iterating the keys to find the necessary value.
- **Objects/arrays not requiring temporary storage to serialize**:
	- This would lead to a huge increase in overall complexity and size due to requiring a linked/skip list of object fields/array elements. This linked list would also make it impossible to do a binary search on object keys.
- **Putting types with the value (as opposed to with the object that refers to the value)**:
	- This removes the possibility of inlining smaller values effectively
		- For example, currently storing any 32-bit value requires 1 byte for type and 4 bytes for the value.
		- By storing the type with the value, it would be needed to store 4 bytes for the reference + the 1+4 bytes listed above.
		- As a result, it would be necessary to consider smaller types for smaller values, which would then increase the number of integer casting implementations needed.
- **Using 64-bit values and references**:
	- Most use cases do not require these and would therefore increase the size of the file for most uses for the benefit of few uses (that are unlikely to need this format anyway).
	- That said, it would also remove some complexity around integer casting which is why it was considered in the first place.
	- This may be revisited in the future if it's found to be beneficial.
- **More (elaborate) options for variable-length size encoding**:
	- Since in some parser implementations the size of arrays, objects, and their keys may be retrieved from the file many times, it was important that this was still fast enough to do. Variable length integers reduce performance by creating dependencies between each parsing step, which requires the CPU to see the results of each operation before it can do the next thing. Even the U8+U32 encoding was found to make parsing slower by about 15% compared to the other options.
- **Multiple (or flagged) object/array/etc. types for different length encodings**:
	- The idea was that e.g. one array type had an uint8 length value and another one had uint32.
	- This would make a parser more difficult to test by creating more code paths that would be rarely used (which would make them prone to breaking at the exact point when they become useful).
	- This would also reduce the number of available indices for additional types, although we are unlikely to run out of them soon - except in the case when flagging is used (such as by dedicating some top bits for different encodings).
