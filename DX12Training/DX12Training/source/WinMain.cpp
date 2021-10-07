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
#include "Engine\Core\Math\Vertex.h"
#include "Engine\Core\Math\Color.h"

#if _DEBUG
int main()
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#endif
{
	// メモリリーク検知
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

#if _DEBUG
	// D3D12のデバッグ関係処理
	ID3D12Debug* debugLayer = nullptr;
	D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
	debugLayer->EnableDebugLayer();
	debugLayer->Release();
#endif

	// D3D12のオブジェクトたち
	ID3D12Device* device = nullptr;
	IDXGIFactory6* factory = nullptr;
	IDXGISwapChain4* swapChain = nullptr;
	ID3D12CommandAllocator* cmdAllocator = nullptr;
	ID3D12GraphicsCommandList* cmdList = nullptr;
	ID3D12CommandQueue* cmdQueue = nullptr;
	D3D_FEATURE_LEVEL featureLevel = (D3D_FEATURE_LEVEL)-1;
	ID3D12RootSignature* rootSignature;
	ID3D12PipelineState* pipelineState;
	ID3D12Resource* vertexBuffer;
	ID3D12Resource* indicesBuffer;
	std::vector<TextureColor> texturedata(256 * 256);

	for (int index = 0; index < 256 * 256; ++index)
	{
		texturedata[index].a = 255;
		texturedata[index].r = rand() % 256;
		texturedata[index].g = rand() % 256;
		texturedata[index].b = rand() % 256;
	}

	// アダプターを列挙するためのオブジェクト
#if _DEBUG
	HRESULT result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory));
#else
	HRESULT result = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
#endif
	// エラーチェック
	if (result != S_OK)
	{
		return -1;
	}

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

	// Windowの生成
	WNDCLASSEX w = {};
	w.cbSize = sizeof(WNDCLASSEX);
	// ウィンドウのメッセージ更新の際の処理を登録
	w.lpfnWndProc = (WNDPROC)Window::WindowProc;
	// Window名
	w.lpszClassName = "DirextX12Training";
	w.hInstance = GetModuleHandle(nullptr);
	RegisterClassEx(&w);
	// サイズ
	RECT windowRect = { 0,0,960,540 };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, false);
	// Windowの生成
	HWND hwnd = CreateWindow(w.lpszClassName, "DirectX12Training", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, nullptr, nullptr, w.hInstance, nullptr);
	// Windowの表示
	ShowWindow(hwnd, SW_SHOW);

	// コマンド系
	result = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator));
	if (result != S_OK)
	{
		return -1;
	}
	result = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator, nullptr, IID_PPV_ARGS(&cmdList));
	if (result != S_OK)
	{
		return -1;
	}

	// コマンドキュー
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};

	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQueueDesc.NodeMask = 0;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	// コマンドキューを生成
	result = device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&cmdQueue));

	// エラーチェック
	if (result != S_OK)
	{
		return -1;
	}

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};

	// スワップチェイン関係
	swapChainDesc.Width = 960;
	swapChainDesc.Height = 540;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	result = factory->CreateSwapChainForHwnd(cmdQueue, hwnd, &swapChainDesc, nullptr, nullptr, (IDXGISwapChain1**)&swapChain);
	if (result != S_OK)
	{
		return -1;
	}

	// ディスクリプタヒープの生成
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	// レンダーターゲットビュー
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	// 複数のGPUの識別を行うためのもの、１つだけなら０でいい
	heapDesc.NodeMask = 0;
	// 表裏の２つ
	heapDesc.NumDescriptors = 2;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	ID3D12DescriptorHeap* rtvHeaps = nullptr;
	result = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));

	if (result != S_OK)
	{
		return -1;
	}

	DXGI_SWAP_CHAIN_DESC swapChainDesc_;
	ZeroMemory(&swapChainDesc_, sizeof(DXGI_SWAP_CHAIN_DESC));
	result = swapChain->GetDesc(&swapChainDesc_);

	if (result != S_OK)
	{
		return -1;
	}

	// スワップチェインのバックバッファ
	std::vector<ID3D12Resource*> backBuffers(swapChainDesc_.BufferCount);

	// バックバッファのRenderTargetを生成
	for (unsigned int index = 0; index < swapChainDesc_.BufferCount; ++index)
	{
		result = swapChain->GetBuffer(index, IID_PPV_ARGS(&backBuffers[index]));

		if (result != S_OK)
		{
			return -1;
		}
		D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();

		handle.ptr += index * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		device->CreateRenderTargetView(backBuffers[index], nullptr, handle);
	}

	// フェンスの生成
	ID3D12Fence* fence = nullptr;
	UINT64 fenceVal = 0;

	result = device->CreateFence(fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	HANDLE eventHandle = CreateEvent(nullptr, false, false, nullptr);

	if (result != S_OK)
	{
		return -1;
	}

	// シェーダーの初期化

	/*Math::Vector3 vertices_arrays[] = {
		{0.0f,1.0f,0.0f},
		{-1.0f,-1.0f,0.0f},
		{1.0f,-1.0f,0.0f},
	};*/

	/*Math::Vector3 vertices_arrays[] = {
	{0.0f,0.7f,0.0f},
	{-0.7f,-0.7f,0.0f},
	{0.7f,-0.7f,0.0f},
	{0.7f,0.7f,0.0f},
	};*/

	unsigned short indices[] = {
		0,1,2,
		2,1,3
	};

	Math::Vertex vertices_arrays[] = {
	{{-0.7f,-0.7f,0.0f},{0.0f,1.0f}},
	{{-0.7f,0.7f,0.0f} ,{0.0f,0.0f}},
	{{0.7f,-0.7f,0.0f},{1.0f,1.0f}},
	{{0.7f,0.7f,0.0f},{1.0f,0.0f}},
	};

	D3D12_ROOT_SIGNATURE_DESC descRootSignature;
	ID3DBlob* rootSigBlob;
	ID3DBlob* errorBlob;
	// ZeroリセットしないとSerialize時にクラッシュする
	ZeroMemory(&descRootSignature, sizeof(D3D12_ROOT_SIGNATURE_DESC));

	D3D12_DESCRIPTOR_RANGE descTableRange;
	ZeroMemory(&descTableRange, sizeof(D3D12_DESCRIPTOR_RANGE));
	descTableRange.NumDescriptors = 1;
	descTableRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descTableRange.BaseShaderRegister = 0;
	descTableRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParameter;
	ZeroMemory(&rootParameter, sizeof(D3D12_ROOT_PARAMETER));
	rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameter.DescriptorTable.pDescriptorRanges = &descTableRange;
	rootParameter.DescriptorTable.NumDescriptorRanges = 1;

	D3D12_STATIC_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc, sizeof(D3D12_STATIC_SAMPLER_DESC));
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

	descRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	descRootSignature.pParameters = &rootParameter;
	descRootSignature.NumParameters = 1;
	descRootSignature.pStaticSamplers = &samplerDesc;
	descRootSignature.NumStaticSamplers = 1;

	result = D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);
	if (result != S_OK)
	{
		return -1;
	}
	result = device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	if (result != S_OK)
	{
		return -1;
	}
	rootSigBlob->Release();

	//FILE* vertexShader = nullptr;
	//fopen_s(&vertexShader, "..\\vertexshader.cso", "rb");
	//if (vertexShader == nullptr)
	//{
	//	return -1;
	//}

	//fseek(vertexShader, 0, SEEK_END);
	//int vertexShaderSize = ftell(vertexShader);
	//rewind(vertexShader);
	//void* vertexShaderPtr = malloc(vertexShaderSize);
	//fread(vertexShaderPtr, 1, vertexShaderSize, vertexShader);
	//fclose(vertexShader);
	//vertexShader = nullptr;

	//FILE* pixelShader = nullptr;
	//fopen_s(&pixelShader, "..\\pixelshader.cso", "rb");
	//if (pixelShader == nullptr)
	//{
	//	return -1;
	//}
	//fseek(pixelShader, 0, SEEK_END);
	//int pixelShaderSize = ftell(pixelShader);
	//rewind(pixelShader);
	//void* pixelShaderPtr = malloc(pixelShaderSize);
	//fread(pixelShaderPtr, 1, pixelShaderSize, pixelShader);
	//fclose(pixelShader);
	//pixelShader = nullptr;

	ID3DBlob* vsBlob = nullptr;
	ID3DBlob* psBlob = nullptr;

	result = D3DCompileFromFile(L"vertexshader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vsBlob, &errorBlob);

	if (result != S_OK)
	{
		return -1;
	}

	result = D3DCompileFromFile(L"pixelshader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &psBlob, &errorBlob);

	if (result != S_OK)
	{
		return -1;
	}

	// 頂点レイアウト
	D3D12_INPUT_ELEMENT_DESC descInputElements[] = {
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC descPipelineState;
	ZeroMemory(&descPipelineState, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	descPipelineState.VS.pShaderBytecode = vsBlob->GetBufferPointer();
	descPipelineState.VS.BytecodeLength = vsBlob->GetBufferSize();
	descPipelineState.PS.pShaderBytecode = psBlob->GetBufferPointer();
	descPipelineState.PS.BytecodeLength = psBlob->GetBufferSize();
	//descPipelineState.SampleDesc.Count = 1;
	//descPipelineState.SampleDesc.Quality = 0;
	//descPipelineState.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	//descPipelineState.InputLayout.pInputElementDescs = descInputElements;
	//descPipelineState.InputLayout.NumElements = _countof(descInputElements);
	descPipelineState.pRootSignature = rootSignature;
	//descPipelineState.NumRenderTargets = 1;
	//descPipelineState.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	//descPipelineState.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//descPipelineState.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	//descPipelineState.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	//descPipelineState.RasterizerState.DepthClipEnable = true;
	//descPipelineState.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	//descPipelineState.RasterizerState.MultisampleEnable = false;
	//for (unsigned int index = 0; index < swapChainDesc.BufferCount; ++index)
	//{
	//	descPipelineState.BlendState.RenderTarget[index].BlendEnable = false;
	//	descPipelineState.BlendState.RenderTarget[index].SrcBlend = D3D12_BLEND_ONE;
	//	//descPipelineState.BlendState.RenderTarget[index].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	//	descPipelineState.BlendState.RenderTarget[index].DestBlend = D3D12_BLEND_ZERO;
	//	descPipelineState.BlendState.RenderTarget[index].BlendOp = D3D12_BLEND_OP_ADD;
	//	descPipelineState.BlendState.RenderTarget[index].SrcBlendAlpha = D3D12_BLEND_ONE;
	//	//descPipelineState.BlendState.RenderTarget[index].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
	//	descPipelineState.BlendState.RenderTarget[index].DestBlendAlpha = D3D12_BLEND_ZERO;
	//	descPipelineState.BlendState.RenderTarget[index].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	//	descPipelineState.BlendState.RenderTarget[index].LogicOpEnable = false;
	//	descPipelineState.BlendState.RenderTarget[index].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	//}
	//descPipelineState.BlendState.AlphaToCoverageEnable = false;
	//descPipelineState.BlendState.IndependentBlendEnable = false;
	//descPipelineState.DepthStencilState.DepthEnable = false;
	//descPipelineState.DepthStencilState.StencilEnable = false;

	descPipelineState.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	descPipelineState.BlendState.AlphaToCoverageEnable = false;
	descPipelineState.BlendState.IndependentBlendEnable = false;
	
	ZeroMemory(&descPipelineState.BlendState.RenderTarget[0], sizeof(D3D12_RENDER_TARGET_BLEND_DESC));
	descPipelineState.BlendState.RenderTarget[0].BlendEnable = false;
	descPipelineState.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	descPipelineState.BlendState.RenderTarget[0].LogicOpEnable = false;

	descPipelineState.RasterizerState.MultisampleEnable = false;
	descPipelineState.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	descPipelineState.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	descPipelineState.RasterizerState.DepthClipEnable = true;

	descPipelineState.RasterizerState.FrontCounterClockwise = false;
	descPipelineState.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	descPipelineState.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	descPipelineState.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	descPipelineState.RasterizerState.AntialiasedLineEnable = false;
	descPipelineState.RasterizerState.ForcedSampleCount = 0;
	descPipelineState.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	descPipelineState.DepthStencilState.DepthEnable = false;
	descPipelineState.DepthStencilState.StencilEnable = false;

	descPipelineState.InputLayout.pInputElementDescs = descInputElements;
	descPipelineState.InputLayout.NumElements = _countof(descInputElements);

	descPipelineState.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	descPipelineState.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	descPipelineState.NumRenderTargets = 1;
	descPipelineState.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	descPipelineState.SampleDesc.Count = 1;
	descPipelineState.SampleDesc.Quality = 0;

	result = device->CreateGraphicsPipelineState(&descPipelineState, IID_PPV_ARGS(&pipelineState));

	if (result != S_OK)
	{
		return -1;
	}

	D3D12_HEAP_PROPERTIES heapProperties;
	D3D12_RESOURCE_DESC descResource;
	ZeroMemory(&heapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	ZeroMemory(&descResource, sizeof(D3D12_RESOURCE_DESC));
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 0;
	heapProperties.VisibleNodeMask = 0;
	descResource.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	descResource.Width = sizeof(vertices_arrays);
	descResource.Height = 1;
	descResource.DepthOrArraySize = 1;
	descResource.MipLevels = 1;
	descResource.Format = DXGI_FORMAT_UNKNOWN;
	descResource.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	descResource.SampleDesc.Count = 1;

	result = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &descResource, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer));
	
	if (result != S_OK)
	{
		return -1;
	}

	Math::Vertex* mapped = nullptr;
	//void* mapped = nullptr;
	// HeapProperties.TypeがDEFAULTの状態でMapを呼ぶと失敗する
	result = vertexBuffer->Map(0, nullptr, (void**)&mapped);
	if (result != S_OK)
	{
		return -1;
	}
	std::copy(std::begin(vertices_arrays), std::end(vertices_arrays), mapped);
	vertexBuffer->Unmap(0, nullptr);

	descResource.Width = sizeof(indices);

	result = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &descResource, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indicesBuffer));

	if (result != S_OK)
	{
		return -1;
	}

	unsigned short* indexMapped = nullptr;
	result = indicesBuffer->Map(0, nullptr, (void**)&indexMapped);
	if (result != S_OK)
	{
		return -1;
	}
	std::copy(std::begin(indices), std::end(indices), indexMapped);
	indicesBuffer->Unmap(0, nullptr);

	D3D12_HEAP_PROPERTIES textureHeapProperties;
	ZeroMemory(&textureHeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	textureHeapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;
	textureHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	textureHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	textureHeapProperties.CreationNodeMask = 0;
	textureHeapProperties.VisibleNodeMask = 0;
	D3D12_RESOURCE_DESC textureDescResource;
	ZeroMemory(&textureDescResource, sizeof(D3D12_RESOURCE_DESC));
	textureDescResource.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDescResource.Width = 256;
	textureDescResource.Height = 256;
	textureDescResource.DepthOrArraySize = 1;
	textureDescResource.SampleDesc.Count = 1;
	textureDescResource.SampleDesc.Quality = 0;
	textureDescResource.MipLevels = 1;
	textureDescResource.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDescResource.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDescResource.Flags = D3D12_RESOURCE_FLAG_NONE;

	ID3D12Resource* textureBuffer;
	result = device->CreateCommittedResource(&textureHeapProperties, D3D12_HEAP_FLAG_NONE, &textureDescResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, IID_PPV_ARGS(&textureBuffer));

	if (result != S_OK)
	{
		return -1;
	}

	result = textureBuffer->WriteToSubresource(0, nullptr, texturedata.data(), sizeof(TextureColor) * 256, sizeof(TextureColor) * texturedata.size());

	if (result != S_OK)
	{
		return -1;
	}

	ID3D12DescriptorHeap* textureDescHeap;
	D3D12_DESCRIPTOR_HEAP_DESC textureDescHeapDesc;
	textureDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	textureDescHeapDesc.NodeMask = 0;
	textureDescHeapDesc.NumDescriptors = 1;
	textureDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	
	result = device->CreateDescriptorHeap(&textureDescHeapDesc, IID_PPV_ARGS(&textureDescHeap));

	if (result != S_OK)
	{
		return -1;
	}

	D3D12_VERTEX_BUFFER_VIEW bufferPosition;
	bufferPosition.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	bufferPosition.StrideInBytes = sizeof(vertices_arrays[0]);
	bufferPosition.SizeInBytes = sizeof(vertices_arrays);

	D3D12_INDEX_BUFFER_VIEW indexView;
	indexView.BufferLocation = indicesBuffer->GetGPUVirtualAddress();
	indexView.Format = DXGI_FORMAT_R16_UINT;
	indexView.SizeInBytes = sizeof(indices);

	D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;

	device->CreateShaderResourceView(textureBuffer, &shaderResourceViewDesc, textureDescHeap->GetCPUDescriptorHandleForHeapStart());

	float clearColor[] = { 1.0f,0.0f,0.0f,1.0f };
	// ループ周り
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

		// 現在フレームのバックバッファのインデックス値 (0or1)
		int targetIndex = swapChain->GetCurrentBackBufferIndex();

		D3D12_VIEWPORT viewport;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = 960;
		viewport.Height = 540;
		viewport.MinDepth = 0;
		viewport.MaxDepth = 1;

		// バリア設定
		D3D12_RESOURCE_BARRIER barrierDesc = {};
		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrierDesc.Transition.pResource = backBuffers[targetIndex];
		barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		cmdList->ResourceBarrier(1, &barrierDesc);

		D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();

		handle.ptr += targetIndex * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		cmdList->SetPipelineState(pipelineState);
		// RenderTargetを設定
		cmdList->OMSetRenderTargets(1, &handle, true, nullptr);

		// 画面をクリア
		cmdList->ClearRenderTargetView(handle, clearColor, 0, nullptr);

		D3D12_RECT rect{ 0,0,960,540 };
		cmdList->RSSetViewports(1, &viewport);
		cmdList->RSSetScissorRects(1, &rect);
		cmdList->SetGraphicsRootSignature(rootSignature);

		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		cmdList->IASetVertexBuffers(0, 1, &bufferPosition);
		cmdList->IASetIndexBuffer(&indexView);

		cmdList->SetGraphicsRootSignature(rootSignature);
		cmdList->SetDescriptorHeaps(1, &textureDescHeap);
		cmdList->SetGraphicsRootDescriptorTable(0, textureDescHeap->GetGPUDescriptorHandleForHeapStart());

		//cmdList->DrawInstanced(4, 1, 0, 0);
		cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);

		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		cmdList->ResourceBarrier(1, &barrierDesc);

		// 命令を閉じる
		result = cmdList->Close();

		// 命令を実行
		ID3D12CommandList* cmdLists[] = { cmdList };
		cmdQueue->ExecuteCommandLists(1, cmdLists);
		// 待機
		result = cmdQueue->Signal(fence, fenceVal);

		if (fence->GetCompletedValue() != fenceVal)
		{
			result = fence->SetEventOnCompletion(fenceVal, eventHandle);
			WaitForSingleObject(eventHandle, INFINITE);
		}
		++fenceVal;

		// 命令をリセット
		result = cmdAllocator->Reset();
		result = cmdList->Reset(cmdAllocator, nullptr);

		// スワップを実行
		result = swapChain->Present(1, 0);

	}
	// 登録したclassの登録解除
	UnregisterClass(w.lpszClassName, w.hInstance);
	// イベントハンドルを閉じる
	CloseHandle(eventHandle);
	//mallocで確保したメモリを解放する
	/*free(vertexShaderPtr);
	free(pixelShaderPtr);*/

	return (int)msg.wParam;
}