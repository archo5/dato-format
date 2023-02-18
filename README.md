# DATO: Directly Accessing a Tree of Objects
An immediately traversable binary file format for encoding extensible data

## Why this format?

Most file formats exist in one of two categories:

- Formats that require two or more passes to extract data (JSON, XML);
	- These are inherently slow to parse.
- Formats that require matching schemas between parsing and writing (RIFF, flatbuffers);
	- These need to have prebuilt upgrade paths, which are typically very limited.

This format combines the following properties:

- **Direct access** - the data can be accessed directly, without pre-parsing it, for reduced total parsing time
	- The data can be optionally aligned to further improve parsing speed on CPUs that don't support unaligned loads natively
- **Generic** and **Traversable** - there are various built-in structures for storing most data, which allows the following:
	- Changing the format of any object (string and int map) while retaining backwards compatibility
	- Partially inspecting and validating the data without having to knowing the entire format including names of stored values and special meanings of byte ranges
- **Customizable** - the exact details of the format can be optimized for the particular application:
	- It is possible to add more data types
	- The encoding of various length/size values can be changed to optimize for size or performance
	- Map keys can be either names (for readability) or 32-bit integers (for reduced size and faster lookups)
		- Integer keys can be hashes, enum values, indices or FOURCC codes
	- Different string types can be used (to match the application's internal string format)
	- The prefix of the file can be changed

## Why should I use this format?

If one or more of the following is true about your files:

- they need to be loaded quickly and/or partially
- they can change over time in ways difficult to anticipate
- it would be cumbersome to produce or maintain an explicit schema for them
- they may need to contain big blobs of data that should not be parsed or copied several times
- they are read more often than written (e.g. read-only compiled assets stored in an application)
- they contain a large amount of floating point numbers

... then those files would very likely benefit from using this format.

## When shouldn't I use this format?

Apart from the obvious (e.g. downsides of a binary format) don't use it if:

- you need the smallest file size or bandwidth usage
	- (it allows hand-optimizing heavy reuse but will compress relatively poorly and create high structural overhead compared to other formats)
	- This includes most use cases where files are constantly sent over the network.
- your data has more structures than numbers
	- While this format can easily store lots of numbers fairly efficiently, it will not be as efficient if there are lots of generic arrays and maps instead.

## Which configuration of the format should I use?

These configurations tend not to significantly affect the reading speed or the compressed size of the files so unless something specific is being done (heavy use of specific types), it probably won't matter.

Therefore, configuration 0 with alignment and key sorting is the usual suggestion as well as the default one - it favors performance and compatibility without limiting usability and inspectability.

Size encoding configurations:

- in the order of most performant to smallest:
- **0**: all sizes are 4 bytes long
	- Optimizes for reading speed at the cost of size
- **1**: key, map and generic array sizes are 4 bytes long, typed array sizes are variable-length (1-5 bytes)
	- Shifts the optimization towards size at a minor cost to reading speed
- **2**: key sizes are 4 bytes long, all other sizes are variable-length (1-5 bytes)
	- Optimizes for size at the cost of reading speed
- Custom configurations can be created as well, including ones that don't support certain sizes of data (for a minor speedup).

Additional options:

- Alignment can be disabled to reduce size at the cost of traversal speed (at least on some platforms).
	- This doesn't seem to affect the compressibility of files however (compressed sizes will be roughly the same).
- Key sorting can be disabled to improve serialization speed at the cost of lookup speed.

## The file format specification (**warning: not finalized at this point - minor details may change**)

```py
# It is encouraged to "bend" the format for your needs, however the following properties are normally expected:
# - Endianness: little endian
# - KEY-STRING content encoding: ASCII/UTF-8/raw bytes, sorted as raw bytes
# - String8 content encoding: ASCII/UTF-8
# - String16 content encoding: UTF-16
# - String32 content encoding: UTF-32
# - String* content encoding notes: no BOM, little-endian where applicable

FILE =
{
	PREFIX-BYTES
	SIZE-ENC-CONFIG-BYTE
	PROPERTY-BYTE
	TYPE
	ALIGN(4)
	VALUE # VREF is instead REF (absolute)
}

PREFIX-BYTES = "DATO" | [user-defined]

SIZE-ENC-CONFIG-BYTE = [0;2] | [128;255]
# values:
# - 0-2 refer to standardized size encoding configurations
# - 3-127 are reserved
# - 128-255 can be used for specifying application-specific configurations

PROPERTY-BYTE = [0 1 R R R R R R]
# values are flags:
# - bit 0 specifies whether the file is aligned (1 = yes)
#	- alignment applies to every element larger than one byte - they are expected ..
#	.. to be placed at an offset that is a multiple of its size, e.g. ..
#	.. uint32 could be placed at offset 24 (4*6) or 52 (4*13) but not 37
# - bit 1 specifies whether the keys are sorted (1 = yes)
#	- integer keys are expected to be sorted by value (uint32), string keys by content (comparing byte values)
# - bits 2-7 are reserved

ALIGN(...) = [empty] ... 0[N]
# if alignment is enabled, this contains 0 or more zero-bytes, to align the in-file position of each subsequent value contained in the structure to its natural (or explicitly specified) alignment

REF(T) = uint32
# an offset from the start of the file (before the prefix)
# in this spec, T optionally specifies the type that is expected to be at the end of the reference

VREF(T) = uint32
# an offset backwards from the array/map "origin" (aligned offset to data after size)
# absolute offset = array/map origin - VREF-value
# relative references to values tend to compress better since referenced values are typically nearby
# in this spec, T optionally specifies the type that is expected to be at the end of the reference

MAP =
{
	ALIGN(SIZE(MAP), 4) # for mixed-value sizes, the alignment must take into account all the values
	# <- start (where references point to)
	size = SIZE(MAP) # the number of properties in the map
	# <- origin (for relative value refs)
	keys = KEY[size] # no duplicates allowed!
	values = VALUE[size]
	types = TYPE[size]
}

ARRAY =
{
	ALIGN(SIZE(ARRAY), 4) # for mixed-value sizes, the alignment must take into account all the values
	# <- start (where references point to)
	size = SIZE(ARRAY)
	# <- origin (for relative value refs)
	values = VALUE[size]
	types = TYPE[size]
}

TYPED-ARRAY(T, alignment=sizeof(T), null_terminated) =
# in this spec, T optionally specifies the array element type
{
	ALIGN(SIZE(TYPED-ARRAY), alignment) # for mixed-value sizes, the alignment must take into account all the values
	# <- start (where references point to)
	size = SIZE(TYPED-ARRAY)
	values = T[size]
	if null_terminated: T(0)
}
# variations of typed array:
BYTE-ARRAY(alignment) = TYPED-ARRAY(u8, alignment, null_terminated=False)
STRING-8 = TYPED-ARRAY(char8, null_terminated=True)
STRING-16 = TYPED-ARRAY(char16, null_terminated=True)
STRING-32 = TYPED-ARRAY(char32, null_terminated=True)

SIZE(T) = [depends on encoding]
{
	option 1: uint8(n) # currently unused by any of the predefined configurations
	option 2: uint16(n) # currently unused by any of the predefined configurations
	option 3: uint32(n)
	option 4:
	- if n < 255: uint8(n)
	- if n >= 255: uint8(255) uint32(n)
}

KEY = uint32 | REF(KEY-STRING)
# - uint32 is used for IntMap
# - REF(KEY-STRING) is used for StringMap

KEY-STRING =
{
	ALIGN(SIZE(KEY-STRING)) # for mixed-value sizes, the alignment must take into account all the values
	# <- start (where references point to)
	size = SIZE(KEY-STRING) # the number of characters in the key string (excludes the terminating zero)
	data = char[size]
	char(0)
}

VECTOR(T) =
{
	ALIGN(T - 2)
	# <- start (where references point to)
	subtype = SUBTYPE
	vecsize = VECSIZE
	data = T[vecsize]
}

VECTOR-ARRAY(T) =
{
	ALIGN((SIZE(VECTOR-ARRAY), T) - 2) # for mixed-value sizes, the alignment must take into account all the values
	# <- start (where references point to)
	subtype = SUBTYPE
	vecsize = VECSIZE
	count = SIZE(VECTOR-ARRAY)
	values = T[vecsize * count]
}

VECSIZE = uint8
# allowed count range: [1;255]
# 0 is reserved as it may represent 256 in the future

VALUE = int32 | uint32 | float32 | VREF(int64 | uint64 | float64 | ARRAY | MAP | TYPED-ARRAY | VECTOR | VECTOR-ARRAY)

TYPE = uint8
- 0: null (value = 0)
# embedded value types:
- 1: bool (value = 1 | 0)
- 2: int32 (value = input)
- 3: uint32 (value = input)
- 4: float32 (value = input)
# external value reference types:
# - one value:
- 5: int64 (value = VREF(int64(input)))
- 6: uint64 (value = VREF(uint64(input)))
- 7: float64 (value = VREF(float64(input)))
# - generic containers:
- 8: array (value = VREF(ARRAY))
- 9: string map (value = VREF(MAP))
- 10: int map (value = VREF(MAP))
# - raw arrays (identified by purpose)
# (strings contain an extra 0-termination value not included in their size)
- 11: string, 8-bit characters (value = VREF(STRING-8))
- 12: string, 16-bit characters (value = VREF(STRING-16))
- 13: string, 32-bit characters (value = VREF(STRING-32))
- 14: byte array (value = VREF(BYTE-ARRAY))
- 15: vector (value = VREF(VECTOR))
- 16: vector array (value = VREF(VECTOR-ARRAY))
- 17-127: reserved # likely to be used for standardizing frequently used common formats to remove the need to incur the length overhead of putting them into generic typed arrays
- 128-255: application-specific

SUBTYPE = uint8 # first 4 bits and 0-9 only, the remaining values are reserved
- 0: i8
- 1: u8
- 2: i16
- 3: u16
- 4: i32
- 5: u32
- 6: i64
- 7: u64
- 8: f32
- 9: f64
```

## What was considered and rejected for the format?

- **Interleaved map entries**:
	- They required the largest alignment value to be applied for the entire entry (making it 12 bytes big), which effectively added 3 bytes of padding due to the type being 1 byte big initially.
	- For bigger maps, they would make it necessary to pull unused memory into cache before iterating the keys to find the necessary value.
- **Maps/arrays not requiring temporary storage to serialize**:
	- This would lead to a huge increase in overall complexity and size due to requiring a linked/skip list of map/array elements. This linked list would also make it impossible to do a binary search on map keys and a fast element lookup on arrays.
- **Putting types with the value (as opposed to with the map/array that refers to the value)**:
	- This removes the possibility of inlining smaller values effectively
		- For example, currently storing any 32-bit value requires 1 byte for type and 4 bytes for the value.
		- By storing the type with the value, it would be needed to store 4 bytes for the reference + the 1+4 bytes listed above.
		- As a result, it would be necessary to consider smaller types for smaller values, which would then increase the number of number casting implementations needed.
- **Using 64-bit values and references**:
	- Most use cases do not require these and would therefore increase the size of the file for most uses for the benefit of few uses (that are unlikely to need this format anyway).
	- That said, it would also remove some complexity around integer casting which is why it was considered in the first place.
	- This may be revisited in the future if it's found to be beneficial.
- **More (elaborate) options for variable-length size encoding**:
	- Since in some parser implementations the size of arrays, maps, and their keys may be retrieved from the file many times, it was important that this was still fast enough to do. Variable length integers reduce performance by creating dependencies between each parsing step, which requires the CPU to see the results of each operation before it can do the next thing. Even the U8+U32 encoding in some cases was found to make parsing slower by about 15% compared to the other options.
- **Multiple (or flagged) map/array/etc. types for different length encodings**:
	- The idea was that e.g. one array type had an uint8 length value and another one had uint32.
	- This would make a parser more difficult to test by creating more code paths that would be rarely used (which would make them prone to breaking at the exact point when they become useful).
	- This would also reduce the number of available indices for additional types, although we are unlikely to run out of them soon - except in the case when flagging is used (such as by dedicating some top bits for different encodings).
- **Having multiple byte array types with different alignment**:
	- This effectively mostly just makes the parsing harder and does not guarantee the actual alignment (which can be validated manually).
- **Replacing typed arrays with aligned arrays**:
	- This would reduce the inspectability of data using this format and even when using a tool to dump the contents of a file, would require, for example, knowing the typical floating point number values/ranges in hex to see which arrays contain floating point values instead of fairly large integers.
- **Entry point at the end of the file** ([FlexBuffers](https://google.github.io/flatbuffers/flatbuffers_internals.html))
	- The issue is that it's difficult to ensure that we have the entire file if we expect the end of the file (whatever it may be) to just make sense.
- **Adaptive overall sizing of integer vectors defining generic maps/arrays** ([FlexBuffers](https://google.github.io/flatbuffers/flatbuffers_internals.html))
	- [Requires branching to read the contents of the vector across many hot paths.](https://github.com/google/flatbuffers/blob/v23.1.21/include/flatbuffers/flexbuffers.h#L133)
	- Would also require smaller integer types to take full advantage of this, however in most cases maps will contain 32 bit values or in some cases references which effectively negate any benefits from selectively reduced sizing by requiring everything else to be 32 bits wide as well.
