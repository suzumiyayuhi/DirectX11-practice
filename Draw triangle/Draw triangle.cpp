#include<d3d11.h>
#include<D3DX11.h>
#include<Windows.h>
#include<d3dcompiler.h>
#include<xnamath.h>

struct SimpleVertex
{
	XMFLOAT3 vertex;
};

HINSTANCE				g_hInstance;
HWND					g_hWnd;
D3D_DRIVER_TYPE			g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL		g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*			g_pd3dDevice = NULL;
ID3D11DeviceContext*	g_pImmediateContext = NULL;
IDXGISwapChain*			g_pSwapChain = NULL;
ID3D11RenderTargetView*	g_pRenderTargetView = NULL;
ID3D11PixelShader*		g_pPixelShader = NULL;
ID3D11VertexShader*		g_pVertexShader = NULL;
ID3D11InputLayout*		g_pVertexLayout = NULL;
ID3D11Buffer*			g_pVertexBuffer = NULL;

HRESULT InitWindow(HINSTANCE, int nCmdShow);
HRESULT InitDevice();
//HRESULT CompileShaderFromFile(WCHAR*, LPSTR, LPSTR, ID3DBlob**);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void Render();
void CleanUpDevice();

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	if (FAILED(InitWindow(hInstance, nCmdShow)))
		return 0;
	if (FAILED(InitDevice()))
		return 0;

	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Render();
		}
	}
	CleanUpDevice();
	return 0;
}

HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
	WNDCLASSEX wc;
	wc.cbClsExtra = 0;
	wc.cbSize = sizeof(wc);
	wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wc.hIconSm = LoadIcon(hInstance, IDI_APPLICATION);
	wc.hInstance = hInstance;
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = L"DrawTri";
	wc.lpszMenuName = NULL;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	if (!(RegisterClassEx(&wc)))
		return E_FAIL;

	g_hInstance = hInstance;
	g_hWnd = CreateWindow(L"DrawTri", L"Draw Triangle", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL);
	if (!g_hWnd)
	{
		MessageBox(NULL, L"FAIL", 0, 0);
		return E_FAIL;
	}

	ShowWindow(g_hWnd, nCmdShow);
	return S_OK;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;


	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
	ID3DBlob* pErrorBlob;
	
	HRESULT hr = D3DX11CompileFromFile(szFileName, NULL, NULL, szEntryPoint, szShaderModel, dwShaderFlags
		, 0, NULL, ppBlobOut, &pErrorBlob, NULL);
	if (FAILED(hr))
	{
		if (pErrorBlob != NULL)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		if (pErrorBlob)
			pErrorBlob->Release();
		return hr;
	}
	if (pErrorBlob)
		pErrorBlob->Release();
	return S_OK;
}

HRESULT InitDevice()
{
	RECT rect;
	GetClientRect(g_hWnd, &rect);
	UINT width = rect.right - rect.left;
	UINT height = rect.bottom - rect.top;

	UINT createDeviceFlags = 0;

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.OutputWindow = g_hWnd;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	HRESULT hr = S_OK;
	for (int driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		g_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDeviceAndSwapChain(NULL, g_driverType, NULL, createDeviceFlags,
			featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
			&g_pd3dDevice, &g_featureLevel,&g_pImmediateContext);
		if (SUCCEEDED(hr))
			break;
	}
	if (FAILED(hr))
		return hr;

	ID3D11Texture2D* pBackBuffer = NULL;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hr))
		return hr;

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr))
		return hr;

	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);

	D3D11_VIEWPORT vp;
	vp.Height = (float)height;
	vp.Width = (float)width;
	vp.MaxDepth = 1.f;
	vp.MinDepth = 0.f;
	vp.TopLeftX = 0.f;
	vp.TopLeftY = 0.f;
	g_pImmediateContext->RSSetViewports(1, &vp);


	ID3DBlob* pVSBlob = NULL;
	hr = CompileShaderFromFile(L"draw_tri.fx", "VS", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL, L"The fx file cannot be compiled", 0, 0);
		return hr;
	}
	//创建顶点着色器
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(),
		NULL, &g_pVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		
		return hr;
	}
//____________________________________________________________________________________________________
	D3D11_INPUT_ELEMENT_DESC layout[]=
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},

	};
	UINT numElements = ARRAYSIZE(layout);
	
	hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &g_pVertexLayout);
	pVSBlob->Release();
	if (FAILED(hr))
		return hr;

	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

	

	ID3DBlob *pPSBlob = NULL;
	hr = CompileShaderFromFile(L"draw_tri.fx", "PS", "ps_4_0",&pPSBlob);
	if (FAILED(hr))
		return hr;
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(),
		NULL, &g_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;
	
	SimpleVertex vertexes[] =
	{
		XMFLOAT3(0.f,0.5f,0.5f),
		XMFLOAT3(0.5f,-0.5f,0.5f),
		XMFLOAT3(-0.5f,-0.5f,0.5f),
	};

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 3;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertexes;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);
	if (FAILED(hr))
		return hr;

	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	
	g_pImmediateContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	return S_OK;
}

void CleanUpDevice()
{
	if (g_pd3dDevice)
		g_pd3dDevice->Release();
	if (g_pImmediateContext)
		g_pImmediateContext->Release();
	if (g_pPixelShader)
		g_pPixelShader->Release();
	if (g_pRenderTargetView)
		g_pRenderTargetView->Release();
	if (g_pSwapChain)
		g_pSwapChain->Release();
	if (g_pVertexBuffer)
		g_pVertexBuffer->Release();
	if (g_pVertexLayout)
		g_pVertexLayout->Release();
	if (g_pVertexShader)
		g_pVertexShader->Release();
}

void Render()
{
	float ClearColor[4] = { 0.0f,0.0f,1.0f,0.5f };
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);

	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
	g_pImmediateContext->Draw(3, 0);
	g_pSwapChain->Present(0, 0);
}