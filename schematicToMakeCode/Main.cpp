#include <iostream>
#include <string>
#include <experimental/filesystem>
#include <zlib.h>
#include <cstdint>
#include <cassert>
#include <fstream> 
#include <iomanip>
#include "nbt.h"
#include "ts.h"

namespace fs = std::experimental::filesystem;

std::vector<uint8_t> unGzip(const char* inFileName)
{
	std::vector<uint8_t> unzippedData;
	gzFile gz_file = gzopen(inFileName, "rb");
	if (gz_file == NULL)
	{
		std::cout << "Error: Failed to gzopen " << inFileName << std::endl;
		return unzippedData;
	}
	const int CHUNK = 16384;
	char inbuf[CHUNK];

	while (!gzeof(gz_file))
	{
		int bytesRead = gzread(gz_file, (void*)& inbuf, CHUNK);
		unzippedData.insert(unzippedData.end(), inbuf, inbuf + bytesRead);
	}
	gzclose(gz_file);
	return unzippedData;
}

struct iVec3 {
	int x = 0;
	int y = 0;
	int z = 0;

	iVec3() {}

	iVec3(int x, int y, int z) : x(x), y(y), z(z) {}

	bool operator== (iVec3 const& rhs)
	{
		return x == rhs.x && y == rhs.y && z == rhs.z;
	}
};

struct Block
{
	uint8_t id = {};
	uint8_t data = {};

	Block() {}

	Block(uint8_t id, uint8_t data) : id(id), data(data) {}

	bool operator== (Block rhs)
	{
		return id == rhs.id && data == rhs.data;
	}
};

struct HexPrinter
{
	std::vector<uint8_t> buf;
	size_t limit = 4096 * 6;
	size_t suffix = 0;
	fs::path outFilePath;

	template<class T>
	void append(T value) //append with shrinking
	{
		if constexpr (sizeof(T) > 1)
		{
			while (true)
			{
				uint8_t low = (uint8_t)(value & 0b01111111);
				if (low == value) // last encoded chunk, add stop bit
				{
					buf.push_back((uint8_t)(low | 0b10000000));
					return;
				}
				else
					buf.push_back(low); // encode without stop bit

				value >>= 7;
			}
		}
		else
			buf.push_back(value);
	}

	void print()
	{
		fs::path filepath = outFilePath;
		filepath.replace_extension(std::to_string(suffix) + ".ts");
		std::ofstream ofs;
		ofs.open(filepath, std::fstream::out);
		ofs << "let buf : string = \"\"+\n"; // "+" to concat strings in TypeScript file;

		size_t lineLength = 32;
		size_t index = 0;

		while (index + lineLength <= buf.size())
		{
			ofs << "\"";
			for (size_t i = index; i < index + lineLength; ++i)
			{
				ofs << std::hex << std::setfill('0') << std::setw(2) << (uint32_t)buf[i];
			}
			index += lineLength;
			ofs << "\"+\n";
		}
		ofs << "\"";
		for (size_t i = index; i < buf.size(); ++i)
		{
			ofs << std::hex << std::setfill('0') << std::setw(2) << (uint32_t)buf[i];
		}
		ofs << "\";\n";

		ofs << tsCode;
		ofs << ("from ");
		ofs << filepath.filename();
		ofs << tsCode2;
		
		ofs.close();

	}

	void onRun(iVec3 runStart, int runLength, Block runBlock)
	{
		append(runBlock.id);
		append(runBlock.data);
		append((uint32_t)runStart.x);
		append((uint32_t)runStart.y);
		append((uint32_t)runStart.z);
		append((uint32_t)runLength);

		if (buf.size() >= limit)
		{
			print();
			buf.clear();
			++suffix;
		}
	}
};

void Convert(const char* inFileName)
{
	fs::path inFilePath = fs::path(inFileName);
	fs::path outFilePath = fs::path(inFileName);
	outFilePath.replace_extension(".ts");

	std::vector<uint8_t> fileData = unGzip(inFileName);

	Slice<uint8_t> dataSlice{ fileData.data(), fileData.size() };

	iVec3 size;
	Slice<uint8_t> blocks;
	Slice<uint8_t> data;
	
	std::function<void(Slice<uint8_t>&, NbtTag)> visitor = [&](Slice<uint8_t>& input, NbtTag tag) {
		if (tag.name == "Height")
			size.y = (int32_t)tag.integer;
		else if (tag.name == "Length")
			size.z = (int32_t)tag.integer;
		else if (tag.name == "Width")
			size.x = (int32_t)tag.integer;
		else if (tag.name == "Blocks")
			blocks = readBytes(input, tag.length);
		else if (tag.name == "Data")
			data = readBytes(input, tag.length);
		else
			visitNbtValue(input, tag, visitor);
	};
	visitNbtStream(dataSlice, visitor);

	Block runBlock;
	iVec3 runStart;
	int runLength;

	HexPrinter hexPrinter;
	hexPrinter.outFilePath = outFilePath;

	std::function<void(int, int, int, Block)> visitBlock = [&](int x, int y, int z, Block block)
	{
		if (block == runBlock && runStart == iVec3(x - runLength, y, z))
			++runLength;
		else
		{
			if (runLength)
				hexPrinter.onRun(runStart, runLength, runBlock);

			runBlock = block;
			runLength = 1;
			runStart = iVec3(x, y, z);
		}
	};

	size_t i = 0;
	for (int y = 0; y < size.y; ++y)
	for (int z = 0; z < size.z; ++z)
	for (int x = 0; x < size.x; ++x)
	{
		Block block = Block(blocks[i], data[i]);
		visitBlock(x, y, z, block);
		++i;
	}
	visitBlock(size.x, size.y - 1, size.z - 1, Block(0, 0)); // For the last run

	hexPrinter.print();
}


int main(int argc, char* argv[])
{
	for (int i = 1; i < argc; i++)
	{
		Convert(argv[i]);
	}
}

