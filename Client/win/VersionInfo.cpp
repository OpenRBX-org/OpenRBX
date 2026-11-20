#include "VersionInfo.h"

#ifdef _WIN32
#include <cstdio>
#include <vector>
#include <windows.h>

std::string fileVersionString()
{
	char fileName[MAX_PATH];
	GetModuleFileNameA(nullptr, fileName, MAX_PATH);

	DWORD handle;
	DWORD size = GetFileVersionInfoSizeA(fileName, &handle);
	if (size == 0) {
		return std::string();
	}

	std::vector<BYTE> data(size);
	if (!GetFileVersionInfoA(fileName, handle, size, data.data())) {
		return std::string();
	}

	struct Translation
	{
		WORD language;
		WORD codePage;
	};

	Translation* translations = nullptr;
	UINT length = 0;
	if (!VerQueryValueA(data.data(), "\\VarFileInfo\\Translation", reinterpret_cast<LPVOID*>(&translations), &length) ||
		length < sizeof(Translation)) {
		return std::string();
	}

	char query[64];
	std::snprintf(
		query,
		sizeof(query),
		"\\StringFileInfo\\%04x%04x\\FileVersion",
		translations[0].language,
		translations[0].codePage
	);

	const char* value = nullptr;
	if (!VerQueryValueA(data.data(), query, reinterpret_cast<LPVOID*>(const_cast<char**>(&value)), &length)) {
		return std::string();
	}
	return value;
}
#else
std::string fileVersionString()
{
	return "0, 3, 676, 0";
}
#endif
