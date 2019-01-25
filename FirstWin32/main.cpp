// headers
#include <windows.h>

#include <d3d11.h>

#include <string>

// libraries
#pragma comment(lib, "d3d11.lib")

// global variable
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

// governors variable
int height(1080); // h window
int width(1920); // w window
bool modeScreen(false); // mode screen(true - ful screen, false - windowed mode)

// constants
#define WM_RENDER WM_USER + 1001

// functions
LRESULT WINAPI WndProc(HWND, UINT, WPARAM, LPARAM);
void createDescsTarget(int w, int h);
bool createTargetRender(int w, int h);
bool renderStateEdit(const D3D11_FILL_MODE& fm);
bool resizeWindow(SIZE p);
void renderEditor();

// main
int WINAPI WinMain(HINSTANCE hInst,	
	HINSTANCE hPrev,				
	LPSTR szCmdLine,				
	int nShowCmd)					
{
	WNDCLASSEX wcx = {0};
	wcx.cbSize = sizeof(WNDCLASSEX); 
	wcx.style = CS_HREDRAW | CS_VREDRAW;
	wcx.lpfnWndProc = WndProc;
	wcx.hInstance = hInst;	
	wcx.lpszClassName = TEXT("Win32 DirectX");

	if ( !RegisterClassEx(&wcx) ) 
		return 1;
	HWND hWnd = CreateWindowEx(0,
		TEXT("Win32 DirectX"),
		TEXT("Test going to full screen DirectX programm"),
		WS_OVERLAPPEDWINDOW, 
		CW_USEDEFAULT,0,
		width,height,
		0, 
		0,
		hInst,
		0);

	if (!hWnd)    
		return 2;

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
		return 2;

	createDescsTarget(width, height);
	if (!createTargetRender(width, height))
		return 2;
	if (!renderStateEdit(D3D11_FILL_MODE::D3D11_FILL_SOLID))
		return 2;

	hr = swapChain->SetFullscreenState(modeScreen, nullptr);
	if (hr != S_OK)
		return 2;

	ShowWindow(hWnd, nShowCmd);
	UpdateWindow(hWnd);
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
	return( (int)msg.wParam );	
}


LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
		if (p.cx == 0 || p.cy == 0)
		{
			OutputDebugStringA("Error size window!\n");
			PostMessage(hWnd, WM_QUIT, NULL, NULL);
			break;
		}
		if (!resizeWindow(p))
			PostMessage(hWnd, WM_QUIT, NULL, NULL);
		SendMessage(hWnd, WM_RENDER, NULL, NULL);
		break;
	}

	case WM_RENDER:
		renderEditor();
		break;

	} 
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void createDescsTarget(int w, int h)
{
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = w;
	descDepth.Height = h;
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

	vp.Width = (float)w;
	vp.Height = (float)h;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
}

bool createTargetRender(int w, int h)
{
	ID3D11Texture2D* texture(nullptr);
	HRESULT hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&texture);
	hr = device->CreateRenderTargetView(texture, NULL, &backBuffer);
	if (hr != S_OK)
		return false;
	texture->Release();

	if (DepthStancil)
		DepthStancil->Release();
	if (DepthStancilBuffer)
		DepthStancilBuffer->Release();

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

bool resizeWindow(SIZE p)
{
	HRESULT hr(S_OK);

	int w = p.cx;
	int h = p.cy;

	width = w;
	height = h;

	descDepth.Width = w;
	descDepth.Height = h;
	vp.Width = (float)w;
	vp.Height = (float)h;

	deviceContext->OMSetRenderTargets(0, 0, 0); 
	backBuffer->Release();

	hr = swapChain->ResizeBuffers(0, p.cx, p.cy, DXGI_FORMAT_UNKNOWN, 0); 
	if (hr != S_OK)
		return false;
	createTargetRender(w, h);
	return true;
}

void renderEditor()
{
	float color[] = { 0.36f, 0.36f, 0.36f, 1.0f }; // структуры color больше нет
	deviceContext->ClearDepthStencilView(DepthStancilBuffer, D3D11_CLEAR_DEPTH, 1.0f, 0);
	deviceContext->ClearRenderTargetView(backBuffer, color);

	swapChain->Present(0, 0);
}
