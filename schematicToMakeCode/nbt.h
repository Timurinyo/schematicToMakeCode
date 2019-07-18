#pragma once
#include <cstdint>
#include <functional>
#include <cassert>
#include <string>



template <class T>
struct Slice
{
	T* ptr = {};
	size_t length = {};

	Slice(){}

	Slice(std::nullptr_t){}

	Slice(T* ptr, size_t length) : ptr(ptr), length(length) {}

	Slice slice(size_t start, size_t end)
	{
		assert(start <= length);
		assert(end <= length);
		assert(start <= end);
		return Slice{ ptr + start, end - start };
	}

	Slice take(size_t elemNum)
	{
		assert(elemNum <= length);

		Slice result{ ptr, elemNum };

		length -= elemNum;
		ptr += elemNum;

		return result;
	}

	T& front()
	{
		assert(length > 0);
		return ptr[0];
	}

	void popFront()
	{
		assert(length > 0);
		++ptr;
		--length;
	}

	bool empty()
	{
		return length == 0;
	}

	template <class U>
	Slice<U> to()
	{
		size_t bytesNum = length * sizeof(T);
		assert(bytesNum % sizeof(U) == 0);
		return Slice<U>{(U*)ptr, bytesNum / sizeof(U)};
	}

	T& operator[] (size_t index)
	{
		assert(index < length);
		return ptr[index];
	}
};

bool operator== (Slice<char> lhs, std::string const& rhs);

template <class T>
bool operator== (Slice<T> lhs, Slice<T> rhs);


enum class NbtTagType : uint8_t
{
	tag_end,
	tag_byte,
	tag_short,
	tag_int,
	tag_long,
	tag_float,
	tag_double,
	tag_byte_array,
	tag_string,
	tag_list,
	tag_compound,
	tag_int_array,
	tag_long_array
};

struct NbtTag
{
	NbtTagType type = {};
	Slice<char> name = {};
	union
	{
		int64_t integer = {};
		double floating;
		struct
		{
			uint32_t length;
			NbtTagType itemType;
		};
	};

	NbtTag() {}

	NbtTag(NbtTagType type)
	{
		this->type = type;
	}

	NbtTag(NbtTagType type, Slice<char> name)
	{
		this->type = type;
		this->name = name;
	}

	NbtTag(NbtTagType type, Slice<char> name, double floating)
	{
		this->type = type;
		this->name = name;
		this->floating = floating;
	}

	NbtTag(NbtTagType type, Slice<char> name, int64_t integer)
	{
		this->type = type;
		this->name = name;
		this->integer = integer;
	}

	NbtTag(NbtTagType type, Slice<char> name, NbtTagType itemType, int32_t length)
	{
		this->type = type;
		this->name = name;
		this->itemType = itemType;
		this->length = length;
	}
};

NbtTag decodeNbtTag(Slice<uint8_t>& input, NbtTagType type, Slice<char> name);

Slice<uint8_t> readBytes(Slice<uint8_t>& input, size_t length);

template <class T>
T readInteger(Slice<uint8_t>& input);

void visitNbtStream(Slice<uint8_t>& input, std::function<void(Slice<uint8_t>&, NbtTag)> visitor);

void visitNbtValue(Slice<uint8_t>& input, NbtTag tag, std::function<void(Slice<uint8_t>&, NbtTag)> visitor);

void visitNbtList(Slice<uint8_t>& input, std::function<void(Slice<uint8_t>&, NbtTag)> visitor, NbtTagType type, int32_t length);