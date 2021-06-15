#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <iostream>

const auto *exe = "GGST-Win64-Shipping.exe";
const auto *dll = "strivehitboxes.dll";

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
	const auto result = inject();
	Sleep(2000);
	return result ? 0 : 1;
}
