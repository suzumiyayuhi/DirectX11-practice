#include<d3d11.h>
#include<D3DX11.h>
#include<Windows.h>
#include<xnamath.h>
#include<d3dcompiler.h>

struct SimpleVertex
{
	XMFLOAT3 vertex;
};

HWND						g_hWnd;
HINSTANCE					g_hInstance;
D3D_FEATURE_LEVEL			g_featureLevel = D3D_FEATURE_LEVEL_11_0;
D3D_DRIVER_TYPE				g_driverType = D3D_DRIVER_TYPE_NULL;
ID3D11Device*				g_pd3dDevice = NULL;
ID3D11DeviceContext*		g_pImmediateContext = NULL;
ID3D11RenderTargetView*		g_pRenderTarget = NULL;
IDXGISwapChain*				g_pSwapChain = NULL;
ID3D11PixelShader*			g_pPixelShader = NULL;
ID3D11VertexShader*			g_pVertexShader = NULL;
ID3D11InputLayout*			g_pVertexLayout = NULL;
ID3D11Buffer*				g_pBuffer = NULL;


void Render();
void CleanupDevice();
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HRESULT InitWindow(HINSTANCE, UINT);
HRESULT InitDevice();
HRESULT CompileShaderFromFile(WCHAR*, LPCSTR, LPCSTR, ID3DBlob**);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPWSTR cmdLine, int nCmdShow)
{
	if (FAILED(InitWindow(hInstance, nCmdShow)))
		return 0;
	if (FAILED(InitDevice()))
		return 0;

	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, g_hWnd, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Render();
		}
	}
	CleanupDevice();
	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;

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

HRESULT CompileShaderFromFile(WCHAR* fileName, LPCSTR szEntryPoint, LPCSTR szModelSize, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;
	ID3DBlob *errorBlob = NULL;

	hr = D3DX11CompileFromFile(fileName,NULL, NULL, szEntryPoint,szModelSize,0,0,
		NULL, ppBlobOut,&errorBlob,NULL);
	if (FAILED(hr))
	{
		if (errorBlob)
		{
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			errorBlob->Release();
		}
		return hr;
	}
	if(errorBlob)
		errorBlob->Release();
	return S_OK;
}

HRESULT InitWindow(HINSTANCE hInstance, UINT nCmdShow)
{
	WNDCLASSEX wc;
	wc.cbClsExtra = 0;
	wc.cbSize = sizeof(wc);
	wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wc.hIconSm = LoadIcon(hInstance, IDI_APPLICATION);
	wc.hInstance = hInstance;
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = L"draw rec";
	wc.lpszMenuName = NULL;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	if (!RegisterClassEx(&wc))
		return E_FAIL;
	g_hInstance = hInstance;
	g_hWnd = CreateWindowEx(NULL, L"draw rec", L"draw rectangle", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, g_hInstance, NULL);
	if (!g_hWnd)
		return E_FAIL;
	ShowWindow(g_hWnd, nCmdShow);
	return S_OK;
}

HRESULT InitDevice()
{
	RECT rect;
	GetClientRect(g_hWnd, &rect);
	UINT width = rect.right - rect.left;
	UINT height = rect.bottom - rect.top;
	UINT createDeviceFlags = 0;

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
//	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UINT;XXXXX
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.RefreshRate.Numerator = 60;
//	sd.BufferUsage = D3D11_USAGE_DEFAULT;XXXXXX
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = g_hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	HRESULT hr = S_OK;
	for (UINT index = 0; index < numDriverTypes; index++)
	{
		g_driverType = driverTypes[index];
		hr = D3D11CreateDeviceAndSwapChain(NULL, g_driverType, NULL, createDeviceFlags,
			featureLevels, numFeatureLevels,D3D11_SDK_VERSION, &sd, &g_pSwapChain, 
			&g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
		if (SUCCEEDED(hr))
			break;
	}
	if (FAILED(hr))
		return hr;

	ID3D11Texture2D *pBackBuffer = NULL;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hr))
		return hr;

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTarget);
	pBackBuffer->Release();
	if (FAILED(hr))
		return hr;

	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTarget, NULL);

	D3D11_VIEWPORT vp;
	vp.Height = (float)height;
	vp.Width = (float)width;
	vp.MaxDepth = 1.f;
	vp.MinDepth = 0.f;
	vp.TopLeftX = 0.f;
	vp.TopLeftY = 0.f;
	g_pImmediateContext->RSSetViewports(1, &vp);

	ID3DBlob *pVSBlob;
	hr = CompileShaderFromFile(L"draw rec.fx", "VS", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
		return hr;
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}
	D3D11_INPUT_ELEMENT_DESC layOut[] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
	};
	UINT numElements = ARRAYSIZE(layOut);
	hr = g_pd3dDevice->CreateInputLayout(layOut, numElements, pVSBlob->GetBufferPointer()
		, pVSBlob->GetBufferSize(), &g_pVertexLayout);
	pVSBlob->Release();
	if (FAILED(hr))
		return hr;


	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);


	ID3DBlob *pPSBlob = NULL;
	hr = CompileShaderFromFile(L"draw rec.fx", "PS", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
		return hr;
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL,
		&g_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;


	SimpleVertex vertexes[] =
	{
		XMFLOAT3(-0.5f,0.5f,0.5f),
		XMFLOAT3(0.5f,0.5f,0.5f),
		XMFLOAT3(0.5f,-0.5f,0.5f),
		XMFLOAT3(0.5f,-0.5f,0.5f),
		XMFLOAT3(-0.5f,-0.5f,0.5f),
		XMFLOAT3(-0.5f,0.5f,0.5f),
	};
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.ByteWidth = sizeof(SimpleVertex) * 6;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA initData;
	ZeroMemory(&initData, sizeof(initData));
	initData.pSysMem = vertexes;
	hr = g_pd3dDevice->CreateBuffer(&bd, &initData, &g_pBuffer);
	if (FAILED(hr))
		return hr;
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pBuffer, &stride, &offset);
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	return S_OK;
}

void CleanupDevice()
{
	if (g_pBuffer)
		g_pBuffer->Release();
	if (g_pd3dDevice)
		g_pBuffer->Release();
	if (g_pImmediateContext)
		g_pImmediateContext->Release();
	if (g_pPixelShader)
		g_pPixelShader->Release();
	if (g_pRenderTarget)
		g_pRenderTarget->Release();
	if (g_pSwapChain)
		g_pSwapChain->Release();
	if (g_pVertexLayout)
		g_pVertexLayout->Release();
	if (g_pVertexShader)
		g_pVertexShader->Release();
}


void Render()
{
	float clearColor[4] = { 0.f,0.f,1.f,1.f };
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTarget, clearColor);
	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->Draw(6, 0);

	g_pSwapChain->Present(0, 0);
}