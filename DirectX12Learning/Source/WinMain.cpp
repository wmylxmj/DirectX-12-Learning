#include "Core/PrecompiledHeader.h"
#include "Core/CommandQueue.h"
#include "Core/RootSignature.h"
#include "Core/Camera.h"

#include <string>
#include <cassert>
#include <vector>
#include <array>
#include <sstream>

struct ObjectConstants
{
	DirectX::XMFLOAT4X4 WorldViewProj;
};

// 测试用
struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT4 Color;
};

// ----------------全局变量------------------
Camera g_camera;

uint32_t g_viewportWidth = 640;
uint32_t g_viewportHeight = 480;

HWND g_hMainWnd;
Microsoft::WRL::ComPtr<ID3D12Device2> g_device;

// 命令队列系统
std::unique_ptr<CommandQueue> g_pDirectCommandQueue;

// 交换链
Microsoft::WRL::ComPtr<IDXGISwapChain> g_swapChain;

// 交换链缓冲数
const int k_swapChainBufferCount = 2;
Microsoft::WRL::ComPtr<ID3D12Resource> g_swapChainBuffers[k_swapChainBufferCount];
int g_currBackBufferIndex = 0;

Microsoft::WRL::ComPtr<ID3D12Resource> g_depthStencilBuffer;
Microsoft::WRL::ComPtr<ID3D12Resource> g_constantBuffer;

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_rtvDescriptorHeap;
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_dsvDescriptorHeap;
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_cbvDescriptorHeap;

UINT g_rtvDescriptorSize;

BYTE* g_cbMappedData = nullptr;

std::unique_ptr<RootSignature> g_pRootSignature;

Microsoft::WRL::ComPtr<ID3D12PipelineState> g_pso = nullptr;

Microsoft::WRL::ComPtr<ID3D12Resource> g_vertexBufferGPU = nullptr;
Microsoft::WRL::ComPtr<ID3D12Resource> g_indexBufferGPU = nullptr;

D3D12_VERTEX_BUFFER_VIEW g_vbv;
D3D12_INDEX_BUFFER_VIEW g_ibv;

UINT g_indicesCount = 0;

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

bool InitMainWindow(HINSTANCE hInstance) {
	// 创建窗口类
	WNDCLASSEX wc;
	wc.cbSize = sizeof(wc); // 结构的大小
	wc.style = CS_HREDRAW | CS_VREDRAW; // 类样式
	wc.lpfnWndProc = MainWndProc; // 窗口处理函数
	wc.cbClsExtra = 0; // 此类的额外数据量
	wc.cbWndExtra = 0; // 此类型的每个窗口的额外数据量
	wc.hInstance = hInstance; // 应用程序实例的句柄
	wc.hIcon = nullptr; // 32x32大图标
	wc.hCursor = nullptr; // 光标
	wc.hbrBackground = nullptr; // 背景笔刷来设置窗口颜色
	wc.lpszMenuName = nullptr; // 菜单名称
	wc.lpszClassName = L"MainWnd"; // 类名称
	wc.hIconSm = nullptr; // 16x16小图标

	// 注册窗口类
	if (!RegisterClassEx(&wc))
	{
		MessageBox(0, L"Register Window Class Failed.", 0, 0);
		return false;
	}

	// 创建窗口实例
	g_hMainWnd = CreateWindowEx(
		0, // 拓展窗口样式
		wc.lpszClassName, // 窗口类名称
		L"Title", // 窗口标题
		WS_OVERLAPPEDWINDOW, // 窗口样式参数
		CW_USEDEFAULT, CW_USEDEFAULT, g_viewportWidth, g_viewportHeight, // 左上角坐标，宽高
		0, 0, hInstance, 0 // 父窗口句柄、菜单句柄、应用程序实例句柄和一个指向窗口创建数据的指针
	);

	if (!g_hMainWnd)
	{
		MessageBox(0, L"Create Window Failed.", 0, 0);
		return false;
	}

	// 显示并更新窗口
	ShowWindow(g_hMainWnd, SW_SHOW);
	UpdateWindow(g_hMainWnd);

	return true;
}

bool InitDirect3D() {
	// 开启调试层
#if defined(_DEBUG)
	Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
	CHECK_HRESULT(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
#endif

	// 用于创建设备的工厂
	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
	CHECK_HRESULT(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

	// 创建D3D设备 To do: 不满足条件时使用WARP设备
	CHECK_HRESULT(D3D12CreateDevice(
		nullptr, // 默认适配器
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&g_device)
	));

	UINT adapterIndex = 0;
	Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
	while (dxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND) {
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		// 跳过软件适配器（如WARP）
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
			adapterIndex++;
			continue;
		}

		OutputDebugStringW((std::wstring(desc.Description) + std::wstring(L"\n")).c_str());

		adapterIndex++;
		adapter.Reset();
	}
	OutputDebugStringA("节点数量：");
	OutputDebugStringA(std::to_string(g_device->GetNodeCount()).c_str());
	OutputDebugStringA("\n");

	// 创建命令队列系统
	g_pDirectCommandQueue = std::make_unique<CommandQueue>(g_device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT, 0);

	// 创建交换链
	g_swapChain.Reset();
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferDesc.Width = g_viewportWidth;
	swapChainDesc.BufferDesc.Height = g_viewportHeight;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.SampleDesc.Count = 1; // 翻转模型无法使用多重采样
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = k_swapChainBufferCount;
	swapChainDesc.OutputWindow = g_hMainWnd;
	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	CHECK_HRESULT(dxgiFactory->CreateSwapChain(
		g_pDirectCommandQueue->GetCommandQueue(),
		&swapChainDesc,
		g_swapChain.GetAddressOf()
	));

	// 创建 RTV 及 DSV 描述符堆
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = k_swapChainBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	CHECK_HRESULT(g_device->CreateDescriptorHeap(
		&rtvHeapDesc,
		IID_PPV_ARGS(g_rtvDescriptorHeap.GetAddressOf())
	));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	CHECK_HRESULT(g_device->CreateDescriptorHeap(
		&dsvHeapDesc,
		IID_PPV_ARGS(g_dsvDescriptorHeap.GetAddressOf())
	));

	// 创建 RTV
	g_rtvDescriptorSize = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(g_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	for (int i = 0; i < k_swapChainBufferCount; ++i) {
		// 获得交换链中的后台缓冲区
		CHECK_HRESULT(g_swapChain->GetBuffer(i, IID_PPV_ARGS(&g_swapChainBuffers[i])));
		g_device->CreateRenderTargetView(g_swapChainBuffers[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, g_rtvDescriptorSize);
	}

	// 创建深度/模板缓冲区及其视图
	D3D12_RESOURCE_DESC dsvBufferDesc;
	dsvBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	dsvBufferDesc.Alignment = 0;
	dsvBufferDesc.Width = g_viewportWidth;
	dsvBufferDesc.Height = g_viewportHeight;
	dsvBufferDesc.DepthOrArraySize = 1;
	dsvBufferDesc.MipLevels = 1;
	dsvBufferDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	dsvBufferDesc.SampleDesc.Count = 1;
	dsvBufferDesc.SampleDesc.Quality = 0;
	dsvBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	dsvBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	// 优化清除值
	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;
	// 创建深度/模板缓冲区
	CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CHECK_HRESULT(g_device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&dsvBufferDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&clearValue,
		IID_PPV_ARGS(g_depthStencilBuffer.GetAddressOf())
	));
	// 创建视图
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.Texture2D.MipSlice = 0;
	g_device->CreateDepthStencilView(g_depthStencilBuffer.Get(), &dsvDesc, g_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdAllocator = g_pDirectCommandQueue->RequestCommandAllocator();
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList = g_pDirectCommandQueue->RequestCommandList(cmdAllocator.Get());
	CHECK_HRESULT(cmdList->Reset(cmdAllocator.Get(), nullptr));
	// 将深度/模板缓冲区状态转为深度缓冲区
	cmdList->ResourceBarrier(1,
		&RvalueToLvalue(CD3DX12_RESOURCE_BARRIER::Transition(
			g_depthStencilBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_DEPTH_WRITE
		))
	);

	// 发送命令
	g_pDirectCommandQueue->ExecuteCommandList(cmdList.Get());
	g_pDirectCommandQueue->DiscardCommandList(cmdList.Get());
	g_pDirectCommandQueue->DiscardCommandAllocator(g_pDirectCommandQueue->GetFenceValue(), cmdAllocator.Get());
	g_pDirectCommandQueue->WaitForIdle();

	return true;
}

Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	const void* initData,
	UINT64 byteSize,
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
	Microsoft::WRL::ComPtr<ID3D12Resource> defaultBuffer;

	// 建立实际的默认缓冲区资源
	CHECK_HRESULT(device->CreateCommittedResource(
		&RvalueToLvalue(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
		D3D12_HEAP_FLAG_NONE,
		&RvalueToLvalue(CD3DX12_RESOURCE_DESC::Buffer(byteSize)), // Resource Desc
		D3D12_RESOURCE_STATE_COMMON,
		nullptr, // Clear Value
		IID_PPV_ARGS(defaultBuffer.GetAddressOf())
	));

	// 建立中介上传堆，将CPU内存数据通过上传堆复制到默认堆
	CHECK_HRESULT(device->CreateCommittedResource(
		&RvalueToLvalue(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
		D3D12_HEAP_FLAG_NONE,
		&RvalueToLvalue(CD3DX12_RESOURCE_DESC::Buffer(byteSize)),
		D3D12_RESOURCE_STATE_GENERIC_READ, // 允许资源在 CPU 和 GPU 之间进行读取
		nullptr,
		IID_PPV_ARGS(uploadBuffer.GetAddressOf())
	));

	// 描述想要复制到默认堆的数据
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;
	subResourceData.RowPitch = byteSize; // 对于缓冲区，该参数为字节数
	subResourceData.SlicePitch = byteSize; // 对于缓冲区，该参数为字节数

	// CPU 内存数据上传至中介上传堆，再通过上传堆上传至默认堆
	cmdList->ResourceBarrier(1,
		&RvalueToLvalue(CD3DX12_RESOURCE_BARRIER::Transition(
			defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_COPY_DEST
		))
	);
	UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
	cmdList->ResourceBarrier(1,
		&RvalueToLvalue(CD3DX12_RESOURCE_BARRIER::Transition(
			defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_GENERIC_READ
		))
	);

	// 复制完成后才能释放上传堆

	return defaultBuffer;
}

UINT CalcConstantBufferByteSize(UINT byteSize)
{
	// byteSize + 255：如果不是256的倍数则向上调整
	// & ~255：清除低 8 位，确保结果是 256 字节对齐的
	return (byteSize + 255) & ~255;
}

Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
	const std::wstring& filename,
	const D3D_SHADER_MACRO* defines,
	const std::string& entrypoint,
	const std::string& target)
{
	UINT compileFlags = 0;
#if defined(_DEBUG)
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT hr = S_OK;

	Microsoft::WRL::ComPtr<ID3DBlob> byteCode = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errors;

	hr = D3DCompileFromFile(
		filename.c_str(),
		defines,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entrypoint.c_str(), // 入口点
		target.c_str(), // 着色器类型和版本
		compileFlags, // 编译标志选项
		0, // 高级编译选项
		&byteCode,
		&errors
	);

	if (errors != nullptr)
		OutputDebugStringA((char*)errors->GetBufferPointer());

	CHECK_HRESULT(hr);

	return byteCode;
}

bool AppInit() {
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdAllocator = g_pDirectCommandQueue->RequestCommandAllocator();
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList = g_pDirectCommandQueue->RequestCommandList(cmdAllocator.Get());
	CHECK_HRESULT(cmdList->Reset(cmdAllocator.Get(), nullptr));

	// 建立常量缓冲区描述符堆
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	CHECK_HRESULT(g_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&g_cbvDescriptorHeap)));

	// 建立常量缓冲区
	UINT cbByteSize = (sizeof(ObjectConstants) + 255) & ~255;
	CHECK_HRESULT(g_device->CreateCommittedResource(
		&RvalueToLvalue(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
		D3D12_HEAP_FLAG_NONE,
		&RvalueToLvalue(CD3DX12_RESOURCE_DESC::Buffer(cbByteSize)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&g_constantBuffer)
	));
	CHECK_HRESULT(g_constantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&g_cbMappedData)));

	// 创建常量缓冲区视图
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = g_constantBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = CalcConstantBufferByteSize(sizeof(ObjectConstants));
	g_device->CreateConstantBufferView(
		&cbvDesc,
		g_cbvDescriptorHeap->GetCPUDescriptorHandleForHeapStart()
	);

	// 创建根签名
	g_pRootSignature = std::make_unique<RootSignature>(1);
	// 创建根参数
	RootParameter rootParameter;

	// 创建描述符表
	DescriptorRange cbvTable;
	cbvTable.Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		1, // 描述符数量
		0 // 基准着色器寄存器
	);
	rootParameter.InitAsDescriptorTable(
		1, // 描述符区域数量
		&cbvTable // 描述符区域数组的指针
	);

	g_pRootSignature->operator[](0) = rootParameter;

	g_pRootSignature->CreateRootSignature(g_device.Get(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// 编译着色器
	Microsoft::WRL::ComPtr<ID3DBlob> vsByteCode = CompileShader(L"Source\\Shaders\\color.hlsl", nullptr, "VS", "vs_5_1");
	Microsoft::WRL::ComPtr<ID3DBlob> psByteCode = CompileShader(L"Source\\Shaders\\color.hlsl", nullptr, "PS", "ps_5_1");

	// 顶点布局
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	std::wstringstream ss;
	ss << L"输入布局数量：" << inputLayout.size() << std::endl;
	OutputDebugStringW(ss.str().c_str());

	// 定义盒子几何体
	std::array<Vertex, 8> vertices =
	{
		Vertex({ DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::White) }),
		Vertex({ DirectX::XMFLOAT3(-1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Black) }),
		Vertex({ DirectX::XMFLOAT3(+1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Red) }),
		Vertex({ DirectX::XMFLOAT3(+1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Green) }),
		Vertex({ DirectX::XMFLOAT3(-1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Blue) }),
		Vertex({ DirectX::XMFLOAT3(-1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Yellow) }),
		Vertex({ DirectX::XMFLOAT3(+1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Cyan) }),
		Vertex({ DirectX::XMFLOAT3(+1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Magenta) })
	};

	std::array<std::uint16_t, 36> indices =
	{
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};

	g_indicesCount = (UINT)indices.size();

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	Microsoft::WRL::ComPtr<ID3DBlob> VertexBufferCPU = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

	CHECK_HRESULT(D3DCreateBlob(vbByteSize, &VertexBufferCPU));
	CopyMemory(VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	CHECK_HRESULT(D3DCreateBlob(ibByteSize, &IndexBufferCPU));
	CopyMemory(IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	g_vertexBufferGPU = CreateDefaultBuffer(
		g_device.Get(),
		cmdList.Get(),
		vertices.data(),
		vbByteSize,
		VertexBufferUploader
	);

	g_indexBufferGPU = CreateDefaultBuffer(
		g_device.Get(),
		cmdList.Get(),
		indices.data(),
		ibByteSize,
		IndexBufferUploader
	);

	// 创建顶点缓冲区视图
	g_vbv.BufferLocation = g_vertexBufferGPU->GetGPUVirtualAddress();
	g_vbv.StrideInBytes = sizeof(Vertex);
	g_vbv.SizeInBytes = vbByteSize;

	// 创建索引缓冲区视图
	g_ibv.BufferLocation = g_indexBufferGPU->GetGPUVirtualAddress();
	g_ibv.Format = DXGI_FORMAT_R16_UINT;
	g_ibv.SizeInBytes = ibByteSize;

	// 配置管线状态
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { inputLayout.data(), (UINT)inputLayout.size() }; // 输入顶点布局
	psoDesc.pRootSignature = g_pRootSignature->GetRootSignature().Get(); // 根签名
	psoDesc.VS =
	{
		reinterpret_cast<BYTE*>(vsByteCode->GetBufferPointer()),
		vsByteCode->GetBufferSize()
	};
	psoDesc.PS =
	{
		reinterpret_cast<BYTE*>(psByteCode->GetBufferPointer()),
		psByteCode->GetBufferSize()
	};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // 光栅化状态
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT); // 混合状态
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // 深度/模板状态（用于深度/模板测试）
	psoDesc.SampleMask = UINT_MAX; // 禁用采样（禁用位置为0）
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // 图元的拓扑类型
	psoDesc.NumRenderTargets = 1; // 渲染目标数量
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // 渲染目标的格式
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT; // 深度/模板缓冲区的格式
	CHECK_HRESULT(g_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_pso)));

	g_pDirectCommandQueue->ExecuteCommandList(cmdList.Get());
	g_pDirectCommandQueue->DiscardCommandList(cmdList.Get());
	g_pDirectCommandQueue->DiscardCommandAllocator(g_pDirectCommandQueue->GetFenceValue(), cmdAllocator.Get());
	g_pDirectCommandQueue->WaitForIdle();

	return true;
}

void Update() {
	DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();

	g_camera.SetAspectRatio(static_cast<float>(g_viewportWidth) / g_viewportHeight);
	g_camera.RotatePosition(DirectX::XMFLOAT3(0, 0, 0), DirectX::XMFLOAT3(1, 1, 0), 0.01f);
	g_camera.RotateDirection(DirectX::XMFLOAT3(1, 1, 0), 0.01f);

	DirectX::XMMATRIX worldViewProj = world * g_camera.GetViewMatrix() * g_camera.GetProjectionMatrix();

	ObjectConstants objConstants;
	XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));

	memcpy(&g_cbMappedData[0], &objConstants.WorldViewProj, sizeof(ObjectConstants));
}

void Render() {
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdAllocator = g_pDirectCommandQueue->RequestCommandAllocator();
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList = g_pDirectCommandQueue->RequestCommandList(cmdAllocator.Get());
	CHECK_HRESULT(cmdList->Reset(cmdAllocator.Get(), g_pso.Get()));

	CD3DX12_RESOURCE_BARRIER barrier;
	// 清除 Render Target
	// 通知 GPU 资源的状态即将改变
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		g_swapChainBuffers[g_currBackBufferIndex].Get(), // 目标资源：当前后台缓冲区
		D3D12_RESOURCE_STATE_PRESENT, // 当前状态：呈现到屏幕
		D3D12_RESOURCE_STATE_RENDER_TARGET // 目标状态：作为渲染目标使用
	);
	cmdList->ResourceBarrier(1, &barrier);

	// 设置渲染目标
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(
		g_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		g_currBackBufferIndex,
		g_rtvDescriptorSize
	);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsv(g_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	cmdList->OMSetRenderTargets(1, &rtv, true, &dsv);

	// 设置视口
	D3D12_VIEWPORT viewport;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = (FLOAT)g_viewportWidth;
	viewport.Height = (FLOAT)g_viewportHeight;
	viewport.MinDepth = D3D12_MIN_DEPTH;
	viewport.MaxDepth = D3D12_MAX_DEPTH;
	cmdList->RSSetViewports(1, &viewport);

	// 设置裁剪矩形
	D3D12_RECT scissorRect = { 0, 0, (LONG)g_viewportWidth, (LONG)g_viewportHeight };
	cmdList->RSSetScissorRects(1, &scissorRect);

	// 清除渲染目标
	cmdList->ClearRenderTargetView(rtv, DirectX::Colors::LightSteelBlue, 0, nullptr);
	cmdList->ClearDepthStencilView(g_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// 开始渲染
	ID3D12DescriptorHeap* descriptorHeaps[] = { g_cbvDescriptorHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	cmdList->SetGraphicsRootSignature(g_pRootSignature->GetRootSignature().Get());

	cmdList->IASetVertexBuffers(0, 1, &g_vbv);
	cmdList->IASetIndexBuffer(&g_ibv);
	cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	cmdList->SetGraphicsRootDescriptorTable(0, g_cbvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	cmdList->DrawIndexedInstanced(
		g_indicesCount, // 一个实例的索引数量
		1, // INSTANCE COUNT
		0, // START INDEX LOCATION
		0, // BASE VERTEX LOCATION
		0 // START INSTANCE LOCATION
	);

	// 通知 GPU 资源的状态即将改变
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		g_swapChainBuffers[g_currBackBufferIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);
	cmdList->ResourceBarrier(1, &barrier);

	// 提交命令
	g_pDirectCommandQueue->ExecuteCommandList(cmdList.Get());
	g_pDirectCommandQueue->DiscardCommandList(cmdList.Get());
	g_pDirectCommandQueue->DiscardCommandAllocator(g_pDirectCommandQueue->GetFenceValue(), cmdAllocator.Get());

	// 呈现当前的后台缓冲区
	CHECK_HRESULT(g_swapChain->Present(0, 0));

	// 交换前后缓存
	g_currBackBufferIndex = (g_currBackBufferIndex + 1) % k_swapChainBufferCount;

	// 等待命令完成
	g_pDirectCommandQueue->WaitForIdle();
}

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow
) {
	InitMainWindow(hInstance);
	InitDirect3D();
	AppInit();
	OutputDebugStringA("Hello World!\n");
	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			Update();
			Render();
		}
	}
	return 0;
}