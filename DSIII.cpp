#include "DSIII.h"

bool keep_working = true;


uintptr_t GetModuleBaseAddress2(DWORD procId, const wchar_t* modName)
{
	uintptr_t modBaseAddr = 0;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
	if (hSnap != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 modEntry;
		modEntry.dwSize = sizeof(modEntry);
		if (Module32First(hSnap, &modEntry))
		{
			do
			{
				if (!_wcsicmp(modEntry.szModule, modName))
				{
					modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
					break;
				}
			} while (Module32Next(hSnap, &modEntry));
		}
	}
	CloseHandle(hSnap);
	return modBaseAddr;
}


// SPOUT
void Render()
{

}

[[noreturn]] void DSIII::run_worker()
{
	info("Starting worker thread.");
	info("Delayed thread ID: {}", GetCurrentThreadId());


	while (keep_working) {


		if (GetKeyState(VK_F5) & 0x8000) {

			DWORD PID;
			HWND hWnd = FindWindowA(NULL, "DARK SOULS III");
			GetWindowThreadProcessId(hWnd, &PID);
			HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID);

			uintptr_t baseAddr = GetModuleBaseAddress2(PID, L"darksoulsiii.exe");

			uintptr_t ac_client = GetModuleBaseAddress2(PID, L"darksoulsiii.exe");
			uintptr_t base = ac_client + 0x4768E78; //calculate the full base address

			uintptr_t address = 0;
			ReadProcessMemory(hProc, (void*)base, &address, sizeof(address), nullptr); //read the base address
			address += 0x80; //add the offset

			ReadProcessMemory(hProc, (void*)address, &address, sizeof(address), nullptr); //read the base address
			address += 0x1F90; //add the offset

			ReadProcessMemory(hProc, (void*)address, &address, sizeof(address), nullptr); //read the base address
			address += 0x18; //add the offset

			ReadProcessMemory(hProc, (void*)address, &address, sizeof(address), nullptr); //read the base address
			address += 0xD8; //add the offset

			uintptr_t HP = 0;
			ReadProcessMemory(hProc, (void*)address, &HP, sizeof(HP), nullptr); //read the base address

			std::stringstream stream;
			stream << std::hex << &base << "  " << base << "\n";
			stream << std::hex << &address << "  " << address;
			std::string result2(stream.str());
			info("baseAddr: " + result2);
		}

		std::this_thread::yield();
		std::this_thread::sleep_for(16ms);
	}
}

