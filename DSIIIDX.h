#pragma once
#include <Windows.h>
#include <MinHook.h>
#include <string>
#include <algorithm>

#include <iostream>
#include <fstream>
#include <conio.h>
#include <stdio.h>

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


using namespace spdlog;

using namespace DirectX;
using namespace Microsoft::WRL;

// #include "spoutdx.h"


//static long(__stdcall* original_D3D11Present)(IDXGISwapChain*, UINT, UINT) = nullptr;
//static void(__stdcall* original_ClearDepthStencilView)(ID3D11DeviceContext1*, __in  ID3D11DepthStencilView*, __in  UINT, __in  FLOAT, __in  UINT8) = nullptr;


class DSIIIDX
{
public:


	inline static long(__stdcall* original_D3D11Present)(IDXGISwapChain*, UINT, UINT) = nullptr;
	inline static void(__stdcall* original_ClearDepthStencilView)(ID3D11DeviceContext1*, __in  ID3D11DepthStencilView*, __in  UINT, __in  FLOAT, __in  UINT8) = nullptr;


	static long STDMETHODCALLTYPE hooked_D3D11Present(IDXGISwapChain* swap_chain, UINT sync_interval, UINT flags);

	static void STDMETHODCALLTYPE hooked_ClearDepthStencilView(ID3D11DeviceContext1* This,
		/* [annotation] */
		__in  ID3D11DepthStencilView* pDepthStencilView,
		/* [annotation] */
		__in  UINT ClearFlags,
		/* [annotation] */
		__in  FLOAT Depth,
		/* [annotation] */
		__in  UINT8 Stencil);

	bool CreateSharedDX11Texture(ID3D11Device* pd3dDevice,
		unsigned int width,
		unsigned int height,
		DXGI_FORMAT format,
		ID3D11Texture2D** pSharedTexture,
		HANDLE& dxShareHandle);

};

