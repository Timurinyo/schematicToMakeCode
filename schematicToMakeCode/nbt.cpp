#include "nbt.h"
#include <string>
#include <vector>
#include <iostream>
#include <cstdint>
#include <functional>
#include <cassert>



bool operator== (Slice<char> lhs, std::string const& rhs)
{
	Slice<char> convertedRhs{ (char*)rhs.data(), rhs.length() };
	return lhs == convertedRhs;
}

template <class T>
bool operator== (Slice<T> lhs, Slice<T> rhs)
{
	if (lhs.length != rhs.length)
		return false;
	if (lhs.ptr == rhs.ptr)
		return true;
	for (size_t i = 0; i < lhs.length; ++i)
	{
		if (lhs.ptr[i] != rhs.ptr[i])
			return false;
	}
	return true;
}


NbtTag decodeNbtNamedTag(Slice<uint8_t>& input)
{
	NbtTagType type = (NbtTagType)input.front();
	input.popFront();

	if (type == NbtTagType::tag_end)
		return NbtTag(NbtTagType::tag_end);

	uint16_t nameLength = readInteger<uint16_t>(input);
	Slice<char> name = readBytes(input, nameLength).to<char>();

	return decodeNbtTag(input, type, name);	
}

Slice<uint8_t> readBytes(Slice<uint8_t>& input, size_t length)
{
	Slice<uint8_t> result = input.take(length);
	return result;
}

template <class T>
T readInteger(Slice<uint8_t>& input)
{
	static constexpr size_t size = sizeof(T);
	Slice<uint8_t> rawRata{ input.take(size) };

	uint8_t data[size];

	for (int i = 0; i < size; ++i)
	{
		data[i] = rawRata[size - i - 1];
	}

	T result = *(T*)&data;
	return result;
}

NbtTag decodeNbtTag(Slice<uint8_t>& input, NbtTagType type, Slice<char> name)
{
	switch (type)
	{
	case NbtTagType::tag_end:
		return NbtTag(NbtTagType::tag_end);
	case NbtTagType::tag_byte:
		return NbtTag(type, name, (int64_t)readInteger<int8_t>(input));
	case NbtTagType::tag_short:
		return NbtTag(type, name, (int64_t)readInteger<int16_t>(input));
	case NbtTagType::tag_int:
		return NbtTag(type, name, (int64_t)readInteger<int32_t>(input));
	case NbtTagType::tag_long:
		return NbtTag(type, name, (int64_t)readInteger<int64_t>(input));
	case NbtTagType::tag_float:
		return NbtTag(type, name, readInteger<float>(input));
	case NbtTagType::tag_double:
		return NbtTag(type, name, readInteger<double>(input));
	case NbtTagType::tag_byte_array:
		return NbtTag(type, name, NbtTagType::tag_byte, readInteger<int32_t>(input));
	case NbtTagType::tag_string:
		return NbtTag(type, name, (int64_t)readInteger<int16_t>(input));
	case NbtTagType::tag_list:
	{
		NbtTagType itemType = (NbtTagType)readInteger<uint8_t>(input);
		int32_t listLength = readInteger<int32_t>(input);
		return NbtTag(type, name, itemType, listLength);
	}
	case NbtTagType::tag_compound:
		return NbtTag(type, name);
	case NbtTagType::tag_int_array:
		return NbtTag(type, name, NbtTagType::tag_int, readInteger<uint32_t>(input));
	case NbtTagType::tag_long_array:
		return NbtTag(type, name, NbtTagType::tag_long, readInteger<uint32_t>(input));
	default:
		std::cout << (int)type << std::endl;
		assert(false);
	}
}

void visitNbtStream(Slice<uint8_t>& input, std::function<void(Slice<uint8_t>&, NbtTag)> visitor)
{
	while (!input.empty())
	{
		NbtTag tag = decodeNbtNamedTag(input);
		if (tag.type == NbtTagType::tag_end)
			return;
		visitor(input, tag);
	}
}

void visitNbtValue(Slice<uint8_t>& input, NbtTag tag, std::function<void(Slice<uint8_t>&, NbtTag)> visitor)
{
	switch (tag.type)
	{
	case NbtTagType::tag_end: return;
	case NbtTagType::tag_byte: return;
	case NbtTagType::tag_short: return;
	case NbtTagType::tag_int: return;
	case NbtTagType::tag_long: return;
	case NbtTagType::tag_float: return;
	case NbtTagType::tag_double: return;
	case NbtTagType::tag_byte_array: readBytes(input, tag.length); return;
	case NbtTagType::tag_string: readBytes(input, tag.length); return;
	case NbtTagType::tag_list: visitNbtList(input, visitor, tag.itemType, tag.length); return;
	case NbtTagType::tag_compound: visitNbtStream(input, visitor); return;
	case NbtTagType::tag_int_array: readBytes(input, (size_t)tag.length*4); return;
	case NbtTagType::tag_long_array: readBytes(input, (size_t)tag.length*8); return;
	default:
		break;
	}
}

void visitNbtList(Slice<uint8_t>& input, std::function<void(Slice<uint8_t>&, NbtTag)> visitor, NbtTagType type, int32_t length)
{
	for (int32_t i = 0; i < length; ++i)
	{
		NbtTag tag = decodeNbtTag(input, type, nullptr);
	}
}
