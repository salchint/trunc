#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "windows.h"

void usage()
{
	std::cout << "trunc - Truncate file to the given size" << std::endl;
	std::cout << "Usage: trunc <file> <fileLength> [<fileOffset>]" << std::endl;
	std::cout << "Arguments:" << std::endl;
	std::cout << "    file:       The file to truncate." << std::endl;
	std::cout << "    fileLength: The file's desired length in bytes." << std::endl;
	std::cout << "    fileOffset: Optional. If set, the file will be truncated at its beginning and" << std::endl;
	std::cout << "                ending. I.e. what remains from the orginal file will be" << std::endl;
	std::cout << "                <fileLength> bytes starting from byte <fileOffset>. In case" << std::endl;
	std::cout << "                <fileLength> is set to 0, only the beginning will be truncated." << std::endl;
	exit(0);
}

int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		usage();
	}

	auto flen { std::stoll(std::string(argv[2]), 0, 0) };
	auto fbeg { 0ll };

	if (argc == 4)
	{
		fbeg = std::stoll(std::string(argv[3]), 0, 0);
	}

	auto f { CreateFile(argv[1], GENERIC_WRITE, FILE_SHARE_READ, nullptr,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr) };

	if (INVALID_HANDLE_VALUE == f)
	{
		auto rc = GetLastError();
		std::cout << "Failed to open file " << std::quoted(argv[1]) <<
			" for writing: " << rc << std::endl;
		return rc;
	}

	if (0 < fbeg)
	{
		if (0 == flen)
		{
			LARGE_INTEGER fsize { 0 };
			GetFileSizeEx(f, &fsize);
			flen = fsize.QuadPart - fbeg;
		}

		auto fsrc { CreateFile(argv[1], GENERIC_READ, FILE_SHARE_WRITE, nullptr,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr) };

		if (INVALID_HANDLE_VALUE == fsrc)
		{
			auto rc = GetLastError();
			std::cout << "Failed to open file " << std::quoted(argv[1]) <<
				" for reading: " << rc << std::endl;
			return rc;
		}

		auto highLen { 0xffffffffl & (fbeg >> 32) };
		__int64* phighLen { nullptr };
		if (0 < highLen)
		{
			phighLen = &highLen;
		}
		if (INVALID_SET_FILE_POINTER == SetFilePointer(fsrc, 0xffffffff & fbeg,
				reinterpret_cast<long*>(phighLen), FILE_BEGIN))
		{
			auto rc = GetLastError();
			std::cout << "Failed to seek to file position " << fbeg << ": "
				<< rc << std::endl;
			CloseHandle(f);
			return rc;
		}

		DWORD sectorSize { 0 };
		GetDiskFreeSpaceA(nullptr, nullptr, &sectorSize, nullptr, nullptr);
		if (0 == sectorSize) { sectorSize = 512; }

		std::vector<BYTE> buffer(sectorSize);
		auto copyLen { flen };
		auto bytesCount { 0ul };
		auto written { 0ul };
		while (ReadFile(fsrc, buffer.data(), sectorSize, &bytesCount, nullptr)
				&& (0 < copyLen))
		{
			bytesCount = std::min((const int64_t)bytesCount, copyLen);
			WriteFile(f, buffer.data(), bytesCount, &written, nullptr);
			copyLen -= written;
			//std::cout << "Wrote " << written << " bytes, " << copyLen << " to go." << std::endl;
		}
		CloseHandle(fsrc);
	}

	auto highLen { 0xffffffffl & (flen >> 32) };
	__int64* phighLen { nullptr };
	if (0 < highLen)
	{
		phighLen = &highLen;
	}
	if (INVALID_SET_FILE_POINTER == SetFilePointer(f, 0xffffffff & flen,
			reinterpret_cast<long*>(phighLen), FILE_BEGIN))
	{
		auto rc = GetLastError();
		std::cout << "Failed to seek to file position " << flen << ": "
			<< rc << std::endl;
		CloseHandle(f);
		return rc;
	}

	if (0 == SetEndOfFile(f))
	{
		auto rc = GetLastError();
		std::cout << "Failed to set EOF to file position " << flen << ": "
			<< rc << std::endl;
		CloseHandle(f);
		return rc;
	}

	CloseHandle(f);
	return 0;
}

