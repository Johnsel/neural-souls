#include <Windows.h>
#include <MinHook.h>
#include <string>
#include <algorithm>

#include <iostream>
#include <fstream>
#include <conio.h>
#include <stdio.h>

#ifndef _USE_OLD_OSTREAMS
using namespace std;
#endif

#include <memoryapi.h>
#include <iomanip>

#include "logging.h"
#include "spdlog\spdlog.h"

#include <TlHelp32.h>

#include "kiero.h"
#include <dxgi.h>

#include "com_ptr.hpp"

#include <windows.h>
#include <mutex>
#include <exception>

#include <wrl/client.h>
#include <wincodec.h>
#include "ScreenGrab11.h"
#include "DSIII.h"
#include "DSIIIDX.h"

using namespace spdlog;

using namespace DirectX;
using namespace Microsoft::WRL;


namespace fs = std::filesystem;

static fs::path modengine_path;

std::thread m_worker;


struct Patch {
	DWORD64 relAddr;
	DWORD size;
	char patch[50];
	char orig[50];
};

enum GAME {
	DS1,
	DS3,
	SEKIRO,
	UNKNOWN
};

GAME Game;
typedef DWORD64(__cdecl* STEAMINIT)();
STEAMINIT fpSteamInit = NULL;

typedef DWORD64(__stdcall* DIRECTINPUT8CREATE)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
DIRECTINPUT8CREATE fpDirectInput8Create;

extern "C" __declspec(dllexport)  HRESULT __stdcall DirectInput8Create(
	HINSTANCE hinst,
	DWORD dwVersion,
	REFIID riidltf,
	LPVOID * ppvOut,
	LPUNKNOWN punkOuter
)
{
	return fpDirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);
}
struct MatchPathSeparator
{
	bool operator()(char ch) const
	{
		return ch == '\\' || ch == '/';
	}
};
std::string
basename(std::string const& pathname)
{
	return std::string(
		std::find_if(pathname.rbegin(), pathname.rend(),
			MatchPathSeparator()).base(),
		pathname.end());
}

GAME DetermineGame() {
	const int fnLenMax = 200;
	char fnPtr[fnLenMax];
	auto fnLen = GetModuleFileNameA(0, fnPtr, fnLenMax);

	auto fileName = basename(std::string(fnPtr, fnLen));
	std::transform(fileName.begin(), fileName.end(), fileName.begin(), ::tolower);
	if (fileName == "darksouls.exe") {
		return GAME::DS1;
	}
	else if (fileName == "darksoulsiii.exe") {
		return GAME::DS3;
	}
	else if (fileName == "sekiro.exe") {
		return GAME::SEKIRO;
	}
	else {
		return GAME::UNKNOWN;
	}
}



uintptr_t GetModuleBaseAddress(DWORD procId, const wchar_t* modName)
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
				if (!_wcsicmp((wchar_t*)modEntry.szModule, modName))
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

//inline static void(__stdcall* original_ClearDepthStencilView)(ID3D11DeviceContext1*, __in  ID3D11DepthStencilView*, __in  UINT, __in  FLOAT, __in  UINT8) = nullptr;

DWORD64 __cdecl onSteamInit() {
	if (Game == GAME::DS3) {
		wchar_t dll_filename[MAX_PATH];

		// Grab the path to the modengine2.dll file, so we can locate the global
		// configuration from here if it exists.
		if (!GetModuleFileNameW(GetModuleHandle(NULL), dll_filename, MAX_PATH)) {
			return false;
		}

		modengine_path = fs::path(dll_filename).parent_path();

		auto logger = logging::setup_logger(modengine_path);
		spdlog::set_default_logger(logger);

		info("Main thread ID: {}", GetCurrentThreadId());

		const auto installation_path = modengine_path;
		const auto scylla_path = installation_path / "tools" / "scylla";
		const auto injector_path = scylla_path / "InjectorCLIx64.exe";

		if (!exists(injector_path)) {
			error("!exists");
		}

		info("Found valid ScyllaHide injector. Injecting into game.");

		const auto pid = GetCurrentProcessId();

		WCHAR command_line[MAX_PATH];
		if (swprintf_s(command_line, MAX_PATH, L"%s pid:%d ./HookLibraryx64.dll nowait", injector_path.native().c_str(), pid) < 0) {
			error("!swprintf_s");
		}

		//info(L"Starting ScyllaHide injector: {}", command_line);

		STARTUPINFOW si = {};
		PROCESS_INFORMATION pi = {};

		bool result = CreateProcessW(
			nullptr,
			command_line,
			nullptr,
			nullptr,
			false,
			0,
			nullptr,
			scylla_path.native().c_str(),
			&si,
			&pi);

		if (result) {
			spdlog::info("Launched Scyllahide process.");

			// Wait for it to finish
			DWORD res = WaitForSingleObject(pi.hProcess, 2000);
			if (res == WAIT_TIMEOUT) {
				warn("Scyllahide process timed out.");
			}

			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}

		info("Initializing overlay hooks");
		if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success) {
			kiero::bind(8, (void**)&DSIIIDX::original_D3D11Present, (void*)DSIIIDX::hooked_D3D11Present);
			kiero::bind(114, (void**)&DSIIIDX::original_ClearDepthStencilView, (void*)DSIIIDX::hooked_ClearDepthStencilView);
			info("Initialized overlay hooks");
		}
		else {
			info("Failed to initialize overlay hooks");
		}

		//CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)DelayedPatchesStart, NULL, NULL, NULL);
		 m_worker = thread(DSIII::run_worker);
	}
	return fpSteamInit();
}

void SetupD8Proxy() {
	char syspath[320];
	GetSystemDirectoryA(syspath, 320);
	strcat_s(syspath, "\\dinput8.dll");
	auto hMod = LoadLibraryA(syspath);
	fpDirectInput8Create = (DIRECTINPUT8CREATE)GetProcAddress(hMod, "DirectInput8Create");
}

FARPROC steamInitAddr;

void AttachSteamHook() {
	auto steamApiHwnd = GetModuleHandle(L"steam_api64.dll");
	auto steamInitAddr = GetProcAddress(steamApiHwnd, "SteamAPI_Init");

	MH_CreateHook(steamInitAddr, &onSteamInit, reinterpret_cast<LPVOID*>(&fpSteamInit));
	MH_EnableHook(steamInitAddr);
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:

		Game = DetermineGame();
		if (Game == GAME::UNKNOWN) {
			MessageBoxA(0, "Unable to determine game. Valid EXEs are darksouls.exe, darksoulsiii.exe and sekiro.exe", "", 0);
			break;
		}

		MH_Initialize();
		SetupD8Proxy();
		if (Game == GAME::DS3 || Game == GAME::SEKIRO) {
			//These games are packed. Wait until steam init to apply the patches.
			AttachSteamHook();
		}
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		//keep_working = false;
		//MH_DisableHook(steamInitAddr);
		break;
	}
	return TRUE;
}