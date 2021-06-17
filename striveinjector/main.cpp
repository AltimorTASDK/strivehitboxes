#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "cpp-httplib/httplib.h"

#include <json/json.h>

#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <iostream>

const auto *exe = "GGST-Win64-Shipping.exe";
const auto *dll = "strivehitboxes.dll";
const auto *version = "v1.1.1";

bool elevate_privileges()
{
	LUID luid;
	if (!LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &luid)) {
		std::cerr << "LookupPrivilegeValue failed" << std::endl;
		return false;
	}

	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	HANDLE token;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token)) {
		std::cerr << "OpenProcessToken failed" << std::endl;
		return false;
	}

	if (!AdjustTokenPrivileges(token, FALSE, &tp, 0, (TOKEN_PRIVILEGES *)(nullptr), (DWORD *)(nullptr))) {
		std::cerr << "AdjustTokenPrivileges failed" << std::endl;
		return false;
	}

	return true;
}

bool get_newest_version(std::string *out)
{
	httplib::Client client("https://api.github.com");
	const auto result = client.Get("/repos/AltimorTASDK/strivehitboxes/tags");

	if (result->status != 200)
		return false;

	const auto body = result->body;

	Json::CharReaderBuilder builder;
	const auto reader = builder.newCharReader();

	Json::Value root;
	if (!reader->parse(body.c_str(), body.c_str() + body.length(), &root, nullptr))
		return false;

	*out = root[0]["name"].asString();
	return true;
}

int version_compare(const std::string &a, const std::string &b)
{
	if (a.empty() || b.empty())
		return 0;

	// Skip "v" version prefix
	size_t index_a = a[0] == 'v' ? 1 : 0;
	size_t index_b = b[0] == 'v' ? 1 : 0;

	while (true) {
		const auto num_a = strtol(&a[index_a], nullptr, 10);
		const auto num_b = strtol(&b[index_b], nullptr, 10);

		if (num_a > num_b)
			return 1;
		else if (num_a < num_b)
			return -1;

		index_a = a.find('.', index_a);
		index_b = b.find('.', index_b);

		if (index_a != std::string::npos && index_b == std::string::npos)
			return 1;
		else if (index_a == std::string::npos && index_b != std::string::npos)
			return -1;
		else if (index_a == std::string::npos && index_b == std::string::npos)
			return 0;

		index_a++;
		index_b++;
	}
}

void check_version()
{
	std::string newest;
	if (!get_newest_version(&newest)) {
		std::cerr << "Failed to check for updates." << std::endl;
		return;
	}

	if (version_compare(version, newest) >= 0)
		return;

	std::cout << "A newer version (" << newest << ") of this tool is available at ";
	std::cout << "https://github.com/AltimorTASDK/strivehitboxes/releases" << std::endl;
	std::cout << "Your version: " << version << std::endl;
	std::cout << std::endl;
	std::cout << "Press enter to continue anyways." << std::endl;

	std::cin.get();
}

bool inject()
{
	if (!elevate_privileges())
		return false;

	const auto snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(entry);

	Process32First(snap, &entry);
	do {
		constexpr auto mask =
			PROCESS_QUERY_INFORMATION |
			PROCESS_CREATE_THREAD |
			PROCESS_VM_OPERATION |
			PROCESS_VM_READ |
			PROCESS_VM_WRITE;

		const auto proc = OpenProcess(mask, false, entry.th32ProcessID);

		char path[MAX_PATH];
		if (GetModuleFileNameEx(proc, nullptr, path, MAX_PATH) == 0) {
			CloseHandle(proc);
			continue;
		}

		if (strcmp(path + strlen(path) - strlen(exe), exe) != 0) {
			CloseHandle(proc);
			continue;
		}

		char dll_path[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, dll_path);
		strcat_s(dll_path, MAX_PATH, "/");
		strcat_s(dll_path, MAX_PATH, dll);

		const auto size = strlen(dll_path) + 1;
		auto *buf = VirtualAllocEx(proc, nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if (buf == nullptr) {
			std::cerr << "VirtualAllocEx failed" << std::endl;
			return false;
		}

		WriteProcessMemory(proc, buf, dll_path, size, nullptr);
		CreateRemoteThread(proc, nullptr, 0, (LPTHREAD_START_ROUTINE)(LoadLibrary), buf, 0, nullptr);

		CloseHandle(proc);
		CloseHandle(snap);

		std::cout << "Injected" << std::endl;
		return true;
	} while (Process32Next(snap, &entry));

	std::cerr << "Didn't find " << exe << std::endl;
	return false;
}

int main()
{
	check_version();

	const auto result = inject();
	Sleep(2000);
	return result ? 0 : 1;
}
