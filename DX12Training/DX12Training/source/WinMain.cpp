#include <Windows.h>

#if (DEBUG_ENABLE)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define new ::new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#include <iostream>
#include <vector>

#include "Engine\System\System.h"
#include "Engine\Window\Window.h"

#if _DEBUG
int main()
#else
int WINAPI WinMain(HINSTANCE,HINSTANCE,LPSTR,int)
#endif
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	ID3D12Device* device = nullptr;
	IDXGIFactory6* factory = nullptr;
	IDXGISwapChain4* swapChain = nullptr;
	D3D_FEATURE_LEVEL featureLevel = (D3D_FEATURE_LEVEL)-1;

	CreateDXGIFactory1(IID_PPV_ARGS(&factory));

	// 接続されているアダプターをすべて取得する
	std::vector<IDXGIAdapter*> adapters;
	IDXGIAdapter* tmpAdapter = nullptr;
	for (int i = 0; factory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(tmpAdapter);
	}

	IDXGIAdapter* useAdapter = nullptr;

	// 使用するアダプターをNVIDIAのものに限定する
	for (IDXGIAdapter* adapter : adapters)
	{
		DXGI_ADAPTER_DESC adapterDesc = {};
		adapter->GetDesc(&adapterDesc);
		std::wstring strDesc = adapterDesc.Description;
		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			useAdapter = adapter;
			break;
		}
	}

	// 機能レベルを上から順に配列に詰めていく
	std::vector<D3D_FEATURE_LEVEL> featureLevels;
	featureLevels.push_back(D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_12_1);
	featureLevels.push_back(D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_12_0);
	featureLevels.push_back(D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_1);
	featureLevels.push_back(D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_11_0);
	featureLevels.push_back(D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_10_1);
	featureLevels.push_back(D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_10_0);
	featureLevels.push_back(D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_9_3);
	featureLevels.push_back(D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_9_2);
	featureLevels.push_back(D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_9_1);

	// 機能が使用可能な１番高いレベルを使用する
	for (D3D_FEATURE_LEVEL it : featureLevels)
	{
		// デバイスの生成
		// 指定アダプターがない場合は自動で見つかったものを使用する
		if (D3D12CreateDevice(useAdapter, it, IID_PPV_ARGS(&device)) == S_OK)
		{
			featureLevel = it;
			break;
		}
	}
	// どのレベルも使用できないなら終了
	if (featureLevel == -1)
	{
		return -1;
	}

	WNDCLASSEX w = {};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)Window::WindowProc;
	w.lpszClassName = "DirextX12Training";
	w.hInstance = GetModuleHandle(nullptr);
	RegisterClassEx(&w);
	RECT windowRect = { 0,0,960,540 };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, false);
	HWND hwnd = CreateWindow(w.lpszClassName, "DirectX12Training", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, nullptr, nullptr, w.hInstance, nullptr);
	ShowWindow(hwnd, SW_SHOW);

	MSG msg = {};
	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (msg.message == WM_QUIT)
		{
			break;
		}
	}
	UnregisterClass(w.lpszClassName, w.hInstance);
	return 0;
}