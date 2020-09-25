#include <iostream>
#include <iomanip>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include "windows.h"

void usage()
{
	std::cout << "trunc - Truncate file to the given size" << std::endl;
	std::cout << "Usage: trunc <file> <fileLength>" << std::endl;
	exit(0);
}

int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		usage();
	}

	auto flen { std::stoll(std::string(argv[2])) };

	auto f { CreateFile(argv[1], GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL, nullptr) };

	if (INVALID_HANDLE_VALUE == f)
	{
		auto rc = GetLastError();
		std::cout << "Failed to open file " << std::quoted(argv[1]) <<
			" for writing: " << rc << std::endl;
		return rc;
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

