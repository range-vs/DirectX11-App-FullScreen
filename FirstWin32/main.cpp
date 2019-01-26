// headers
#include <windows.h>

#include <d3d11.h>

#include <string>

// libraries
#pragma comment(lib, "d3d11.lib")

// global variable
// dx
IDXGISwapChain* swapChain(nullptr);
ID3D11Device* device(nullptr);
ID3D11DeviceContext* deviceContext(nullptr);
ID3D11RenderTargetView* backBuffer(nullptr);
ID3D11Texture2D* DepthStancil(nullptr);
ID3D11DepthStencilView* DepthStancilBuffer(nullptr);
ID3D11RasterizerState* rasterizer(nullptr);
D3D11_TEXTURE2D_DESC descDepth; 
D3D11_DEPTH_STENCIL_VIEW_DESC descDSV; 
D3D11_VIEWPORT vp; 

// winapi
HWND hWnd;
HHOOK ExistingKeyboardProc;
HINSTANCE hInstance;
std::wstring className(L"Win32 DirectX");
WINDOWPLACEMENT fullscreenPlacement = { 0 }; // состо€ние экрана в полном режиме
WINDOWPLACEMENT windowedPlacement = { 0 }; // в оконном

// auxiliary
bool isRender(false); // нужно ли сейчас рендерить

// governors variable
int height(600); // h window
int width(800); // w window
bool modeScreen(false); // mode screen(true - full screen, false - windowed mode)
bool enabelAltEnter(false); // enable mode alt + enter?(true - enable)

// constants windows message
#define WM_RENDER WM_USER + 1001

// functions
LRESULT WINAPI wndProc(HWND, UINT, WPARAM, LPARAM);
void createDescsTarget();
bool createTargetRender();
bool renderStateEdit(const D3D11_FILL_MODE& fm);
int resizeWindow(SIZE p);
void renderEditor();
bool initDirectX();
bool hookKeyboardProc(HINSTANCE hinst);
LRESULT CALLBACK keyboardProcLowLevel(int nCode, WPARAM wParam, LPARAM lParam);
int unHookKeyboardProc();
void clear();

// main
int WINAPI WinMain(HINSTANCE hInst,	
	HINSTANCE hPrev,				
	LPSTR szCmdLine,				
	int nShowCmd)					
{
	hInstance = hInst;
	hookKeyboardProc(hInstance); // set keyboard hook

	WNDCLASSEX wcx = {0};
	wcx.cbSize = sizeof(WNDCLASSEX); 
	wcx.style = CS_HREDRAW | CS_VREDRAW;
	wcx.lpfnWndProc = wndProc;
	wcx.hInstance = hInstance;
	wcx.lpszClassName = (LPCSTR)className.c_str();

	if ( !RegisterClassEx(&wcx) ) 
		return 1;
	hWnd = CreateWindowEx(0,
		(LPCSTR)className.c_str(),
		TEXT("Test going to full screen DirectX programm"),
		WS_OVERLAPPEDWINDOW, 
		300,300,
		width,height,
		0, 
		0,
		hInstance,
		0);

	if (!hWnd)    
		return 2;

	ShowWindow(hWnd, nShowCmd); // сразу показываем!!!

	if (!initDirectX())
		return 2;

	MSG msg;
	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (msg.message == WM_QUIT)
			break;
		else
			SendMessage(hWnd, WM_RENDER, NULL, NULL);
	}

	clear();

	return( (int)msg.wParam );	
}


// realisation all function's
LRESULT WINAPI wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_SIZE:
	{
		SIZE p;
		p.cx = LOWORD(lParam);
		p.cy = HIWORD(lParam);
		std::string out = "size screen: " + std::to_string(p.cx) + ", " + std::to_string(p.cy) + "\n";
		OutputDebugStringA(out.c_str());
		if (resizeWindow(p) == -1)
			PostMessage(hWnd, WM_QUIT, NULL, NULL);
		break;
	}

	case WM_RENDER:
		renderEditor();
		break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_HOME:
			{
				modeScreen = modeScreen ? false : true;
				if (modeScreen) // сохран€ем состо€ние перед переходом в полный экран
				{
					windowedPlacement.length = sizeof(WINDOWPLACEMENT);
					GetWindowPlacement(hWnd, &windowedPlacement);
				}
				HRESULT hr = swapChain->SetFullscreenState(modeScreen, nullptr);
				if (hr != S_OK)
					PostMessage(hWnd, WM_QUIT, NULL, NULL);
				if(!modeScreen) // восстанавливаем после выхода из полноэкранного
					SetWindowPlacement(hWnd, &windowedPlacement);
				break;
			}

		case VK_ESCAPE:
			PostMessage(hWnd, WM_QUIT, NULL, NULL);
			break;

		default:
			break;
		}
		break;

	case WM_ACTIVATE:
		if (modeScreen && IsIconic(hWnd) && WA_ACTIVE == LOWORD(wParam))
		{
			isRender = true;
			SetWindowPlacement(hWnd, &fullscreenPlacement);
			swapChain->SetFullscreenState(true, nullptr);
		}
		break;

	default:
		break;

	} 
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool initDirectX()
{
	// init dx
	DXGI_SWAP_CHAIN_DESC sdesc;
	ZeroMemory(&sdesc, sizeof(sdesc));
	sdesc.BufferCount = 1;
	sdesc.OutputWindow = hWnd;
	sdesc.SampleDesc.Count = 4;
	sdesc.SampleDesc.Quality = 0;
	sdesc.Windowed = true;
	sdesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sdesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sdesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	sdesc.BufferDesc.Height = height;
	sdesc.BufferDesc.Width = width;

	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	UINT flags(NULL);
#ifdef DEBUG
	flags = D3D11_CREATE_DEVICE_DEBUG;
#endif
	flags |= D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT;
	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE,
		NULL, flags, levels, 8, D3D11_SDK_VERSION, &sdesc,
		&swapChain, &device, NULL, &deviceContext);
	if (hr != S_OK)
		return false;

	if (!enabelAltEnter) // disable mode alt + enter
	{
		IDXGIDevice * pDXGIDevice;
		HRESULT hr = device->QueryInterface(__uuidof(IDXGIDevice), (void **)&pDXGIDevice);
		if (hr != S_OK)
			return false;

		IDXGIAdapter * pDXGIAdapter;
		hr = pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&pDXGIAdapter);
		if (hr != S_OK)
			return false;

		IDXGIFactory * pIDXGIFactory;
		hr = pDXGIAdapter->GetParent(__uuidof(IDXGIFactory), (void **)&pIDXGIFactory);
		if (hr != S_OK)
			return false;

		hr = pIDXGIFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
		if (hr != S_OK)
			return false;
	}

	isRender = true;

	if (resizeWindow({ width, height }) == -1)
		return false;
	if (!renderStateEdit(D3D11_FILL_MODE::D3D11_FILL_SOLID))
		return false;

	hr = swapChain->SetFullscreenState(modeScreen, nullptr);
	if (hr != S_OK)
		return false;

	return true;
}

bool hookKeyboardProc(HINSTANCE hinst)
{
	ExistingKeyboardProc = SetWindowsHookEx(
		WH_KEYBOARD_LL,	
		keyboardProcLowLevel,
		hinst,
		NULL);	

	if (!ExistingKeyboardProc)
		return false;
	else
		return true;
}

LRESULT CALLBACK keyboardProcLowLevel(int nCode, WPARAM wParam, LPARAM lParam)
{
	KBDLLHOOKSTRUCT * hookstruct = (KBDLLHOOKSTRUCT *)(lParam);

	switch (wParam)
	{
	case WM_KEYDOWN:
		break;
	case WM_SYSKEYDOWN:
		if ((((hookstruct->flags) >> 5) & 1))
		{
			// ALT +
			switch (hookstruct->vkCode)
			{
			case VK_TAB: // ALT+TAB
			{
				if (modeScreen && !IsIconic(hWnd))
				{
					isRender = false;
					fullscreenPlacement.length = sizeof(WINDOWPLACEMENT);
					GetWindowPlacement(hWnd, &fullscreenPlacement);
					ShowWindow(hWnd, SW_SHOWMINNOACTIVE);
					swapChain->SetFullscreenState(false, nullptr);
				}
			}
			break;
			case VK_RETURN: // ALT+ENTER
				break;
			case VK_ESCAPE: // ALT+ESC
				break;
			case VK_DELETE: // ALT+DEL
				break;
			};
		}
		break;
	case WM_KEYUP:
		break;
	case WM_SYSKEYUP:
		break;
	}

	return CallNextHookEx(ExistingKeyboardProc, nCode, wParam, lParam);
}

int unHookKeyboardProc()
{
	if (ExistingKeyboardProc)
	{
		BOOL retcode = UnhookWindowsHookEx((HHOOK)keyboardProcLowLevel);

		if (retcode)
		{
			// Successfully 
		}
		else
		{
			//Error 
		}
		return retcode;
	}
	else
	{
		//Error 
		return -1;
	}
}

void createDescsTarget()
{
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 4;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;

	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
	descDSV.Texture2D.MipSlice = 0;

	vp.Width = (float)width;
	vp.Height = (float)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
}

bool createTargetRender()
{
	if (backBuffer)
		backBuffer->Release();
	if (DepthStancil)
		DepthStancil->Release();
	if (DepthStancilBuffer)
		DepthStancilBuffer->Release();

	HRESULT hr = swapChain->ResizeBuffers(0, 0,0, DXGI_FORMAT_UNKNOWN, 0);
	if (hr != S_OK)
		return false;
	
	ID3D11Texture2D* texture(nullptr);
	hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&texture);
	hr = device->CreateRenderTargetView(texture, NULL, &backBuffer);
	if (hr != S_OK)
		return false;
	texture->Release();

	createDescsTarget();

	hr = device->CreateTexture2D(&descDepth, NULL, &DepthStancil);
	if (hr != S_OK)
		return false;
	hr = device->CreateDepthStencilView(DepthStancil, &descDSV, &DepthStancilBuffer);
	if (hr != S_OK)
		return false;

	deviceContext->OMSetRenderTargets(1, &backBuffer, DepthStancilBuffer);
	deviceContext->RSSetViewports(1, &vp);

	return true;
}

bool renderStateEdit(const D3D11_FILL_MODE& fm)
{
	if (rasterizer)
		rasterizer->Release();
	D3D11_RASTERIZER_DESC wfd;
	ZeroMemory(&wfd, sizeof wfd);
	wfd.FillMode = fm;
	wfd.CullMode = D3D11_CULL_BACK;
	wfd.DepthClipEnable = true;
	HRESULT hr = device->CreateRasterizerState(&wfd, &rasterizer);
	if (hr != S_OK)
		return false;
	return true;
}

int resizeWindow(SIZE p)
{
	if (!isRender)
		return 0;
	if (IsIconic(hWnd))
		return 0;

	width = p.cx;
	height = p.cy;
	if (!createTargetRender())
		return -1;
	SendMessage(hWnd, WM_RENDER, NULL, NULL);
	return 1;
}

void renderEditor()
{
	float color[] = { 0.36f, 0.36f, 0.36f, 1.0f }; 
	deviceContext->ClearDepthStencilView(DepthStancilBuffer, D3D11_CLEAR_DEPTH, 1.0f, 0);
	deviceContext->ClearRenderTargetView(backBuffer, color);

	swapChain->Present(0, 0);
}

void clear()
{
	if (swapChain)
		swapChain->Release();
	if (device)
		device->Release();
	if (deviceContext)
		deviceContext->Release();
	if (backBuffer)
		backBuffer->Release();
	if (DepthStancil)
		DepthStancil->Release();
	if (DepthStancilBuffer)
		DepthStancilBuffer->Release();
	if (rasterizer)
		rasterizer->Release();
	DestroyWindow(hWnd);
	UnregisterClass((LPCSTR)className.c_str(), hInstance);
}
