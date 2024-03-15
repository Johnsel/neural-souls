#include "DSIIIDX.h"

ID3D11Device* device;
ID3D11DeviceContext* context;
int i = 0;

#include "Spout2\SPOUTSDK\SpoutDirectX\SpoutDX\SpoutDX.h" // for sender creation and update

spoutDX spoutDXSender;                         // Sender object


static inline com_ptr<ID3D11Texture2D> texture_from_dsv(ID3D11DepthStencilView* dsv)
{
	if (dsv == nullptr)
		return nullptr;
	com_ptr<ID3D11Resource> resource;
	dsv->GetResource(&resource);
	assert(resource != nullptr);
	com_ptr<ID3D11Texture2D> texture;
	resource->QueryInterface(&texture);
	return texture;
}

//void makeTexture() {
//	D3D11_TEXTURE2D_DESC desc;
//	ZeroMemory(&desc, sizeof(desc));
//	desc.Width = 1024;
//	desc.Height = 1024;
//	desc.MipLevels = 1;
//	desc.ArraySize = 1;
//	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
//	desc.SampleDesc.Count = 1;
//	desc.SampleDesc.Quality = 0;
//	desc.Usage = D3D11_USAGE_DEFAULT;
//	desc.CPUAccessFlags = 0;
//	desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
//	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
//	ID3D11Texture2D* copy_tex;
//	hr = device_A->CreateTexture2D(&desc, NULL, &copy_tex);
//
//
//}


bool first = true;

	long STDMETHODCALLTYPE DSIIIDX::hooked_D3D11Present(IDXGISwapChain* swap_chain, UINT sync_interval, UINT flags)
	{
		if (first) {
			//lifecycle::on_frame.fire(swap_chain, sync_interval, flags);
			swap_chain->GetDevice(__uuidof(ID3D11Device), (void**)&device);

			device->GetImmediateContext(&context);


			if (!spoutDXSender.OpenDirectX11(device))
				error("DX Unable to Open DX11 ref");

			// Optionally give the sender a name
			// If none is specified, the executable name is used
			spoutDXSender.SetSenderName("Simple SendBackBuffer sender");

			first = false;
		}

		spoutDXSender.SendBackBuffer();

		if (GetKeyState(VK_F7) & 0x8000) {

			ComPtr<ID3D11Texture2D> backBuffer;
			HRESULT hr = swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D),
				reinterpret_cast<LPVOID*>(backBuffer.GetAddressOf()));
			if (SUCCEEDED(hr)) {
				hr = SaveDDSTextureToFile(context, backBuffer.Get(),
					L"SCREENSHOT.DDS");

				hr = SaveWICTextureToFile(context, backBuffer.Get(),
					GUID_ContainerFormatJpeg, L"SCREENSHOT.JPG");
			}

		}

		if (GetKeyState(VK_F8) & 0x8000) {


		}

		return original_D3D11Present(swap_chain, sync_interval, flags);
	}



	void STDMETHODCALLTYPE DSIIIDX::hooked_ClearDepthStencilView(ID3D11DeviceContext1* This,
		/* [annotation] */
		__in  ID3D11DepthStencilView* pDepthStencilView,
		/* [annotation] */
		__in  UINT ClearFlags,
		/* [annotation] */
		__in  FLOAT Depth,
		/* [annotation] */
		__in  UINT8 Stencil)
	{
		//info("hooked_ClearDepthStencilView\n");

		if (GetKeyState(VK_F9) & 0x8000) {
			//info("GetKeyState(VK_F9)\n");

			com_ptr<ID3D11Texture2D> dsv_texture = texture_from_dsv(pDepthStencilView);
			D3D11_TEXTURE2D_DESC pDesc;
			dsv_texture->GetDesc(&pDesc);
			//info("pDesc.Width: %d \n", pDesc.Width);

			if (pDesc.Width == 1280) {

				wchar_t buf[100];
				int len = swprintf_s(buf, 100, L"%s%d%s", L"SCREENSHOT", i++, L".DDS");


				HRESULT hr = SaveDDSTextureToFile(context, dsv_texture.get(), buf);
				info("Wrote (presumable) depth buffer to file\n");
			}

		}

		return original_ClearDepthStencilView(This, pDepthStencilView, ClearFlags, Depth, Stencil);
	}



	bool DSIIIDX::CreateSharedDX11Texture(ID3D11Device* pd3dDevice,
		unsigned int width,
		unsigned int height,
		DXGI_FORMAT format,
		ID3D11Texture2D** pSharedTexture,
		HANDLE& dxShareHandle)
	{
		ID3D11Texture2D* pTexture;

		if (pd3dDevice == NULL)
			MessageBoxA(NULL, "CreateSharedDX11Texture NULL device", "SpoutSender", MB_OK);

		//
		// Create a new shared DX11 texture
		//

		pTexture = *pSharedTexture; // The texture pointer

		// if(pTexture == NULL) MessageBoxA(NULL, "CreateSharedDX11Texture NULL texture", "SpoutSender", MB_OK);

		// Textures being shared from D3D9 to D3D11 have the following restrictions (LJ - D3D11 to D3D9 ?).
		//		Textures must be 2D
		//		Only 1 mip level is allowed
		//		Texture must have default usage
		//		Texture must be write only	- ?? LJ ??
		//		MSAA textures are not allowed
		//		Bind flags must have SHADER_RESOURCE and RENDER_TARGET set
		//		Only R10G10B10A2_UNORM, R16G16B16A16_FLOAT and R8G8B8A8_UNORM formats are allowed - ?? LJ ??
		//		** If a shared texture is updated on one device ID3D11DeviceContext::Flush must be called on that device **

		// http://msdn.microsoft.com/en-us/library/windows/desktop/ff476903%28v=vs.85%29.aspx
		// To share a resource between two Direct3D 11 devices the resource must have been created
		// with the D3D11_RESOURCE_MISC_SHARED flag, if it was created using the ID3D11Device interface.
		//
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = width;
		desc.Height = height;
		desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED; // This texture will be shared
		// A DirectX 11 texture with D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX is not compatible with DirectX 9
		// so a general named mutext is used for all texture types
		desc.Format = format;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.SampleDesc.Quality = 0;
		desc.SampleDesc.Count = 1;
		desc.MipLevels = 1;
		desc.ArraySize = 1;

		HRESULT res = pd3dDevice->CreateTexture2D(&desc, NULL, &pTexture); // pSharedTexture);

		if (res != S_OK) {
			// http://msdn.microsoft.com/en-us/library/windows/desktop/ff476174%28v=vs.85%29.aspx
			printf("CreateTexture2D ERROR : [0x%x]\n", res);
			switch (res) {
			case E_INVALIDARG:
				printf("    E_INVALIDARG \n");
				break;
			case E_OUTOFMEMORY:
				printf("    E_OUTOFMEMORY \n");
				break;
			default:
				printf("    Unlisted error\n");
				break;
			}
			return false;
		}

		// The DX11 texture is created OK
		// Get the texture share handle so it can be saved in shared memory for receivers to pick up
		// When sharing a resource between two Direct3D 10/11 devices the unique handle 
		// of the resource can be obtained by querying the resource for the IDXGIResource 
		// interface and then calling GetSharedHandle.
		IDXGIResource* pOtherResource(NULL);
		if (pTexture->QueryInterface(__uuidof(IDXGIResource), (void**)&pOtherResource) != S_OK) {
			printf("    QueryInterface error\n");
			return false;
		}

		// Return the shared texture handle
		pOtherResource->GetSharedHandle(&dxShareHandle);
		pOtherResource->Release();

		*pSharedTexture = pTexture;

		return true;

	}