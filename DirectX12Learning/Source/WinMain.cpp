#include "Windows.h"
#include "WindowsX.h"

#include <wrl.h> // 用于 Microsoft 的智能指针
#include <comdef.h> // 用于处理 HRESULT 错误

// ----------------DirectX 12--------------
#include <dxgi1_6.h>
#include <d3d12.h>
#include <D3Dcompiler.h>

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

#include <DirectXMath.h>
#include "D3D12/d3dx12.h"
// -----------------------------------------

#include <string>
#include <cassert>

// 自己的头文件
#include "D3D12/Helper.h"


struct ObjectConstants
{

};


// ----------------全局变量------------------
uint32_t g_viewportWidth = 640;
uint32_t g_viewportHeight = 480;

HWND g_hMainWnd;
Microsoft::WRL::ComPtr<ID3D12Device2> g_device;

// 用于同步
Microsoft::WRL::ComPtr<ID3D12Fence> g_fence;
uint64_t g_fenceValue = 0;

Microsoft::WRL::ComPtr<ID3D12CommandQueue> g_cmdQueue;
Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> g_cmdList;
// 与后台缓冲区数量相关
Microsoft::WRL::ComPtr<ID3D12CommandAllocator> g_cmdAllocator;
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

void FlushCmdQueue() {
	g_fenceValue++;
	// 发送同步信号
	CHECK_HRESULT(g_cmdQueue->Signal(g_fence.Get(), g_fenceValue));
	// 等待 GPU 命令完成
	if (g_fence->GetCompletedValue() < g_fenceValue) {
		HANDLE eventHandle = CreateEventEx(nullptr, L"Fence Event", false, EVENT_ALL_ACCESS);
		assert(eventHandle != nullptr && "Create Fence Event Failed.");
		CHECK_HRESULT(g_fence->SetEventOnCompletion(g_fenceValue, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
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

	// 创建围栏，用于CPU和GPU同步
	CHECK_HRESULT(g_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)));

	// 创建命令队列 To do: 不同类型的命令队列分开
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	CHECK_HRESULT(g_device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&g_cmdQueue)));

	// 创建命令分配器 To do: 不同类型的命令分配器分开
	CHECK_HRESULT(g_device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(g_cmdAllocator.GetAddressOf())
	));

	// 创建命令列表 To do: 不同类型的命令列表分开
	CHECK_HRESULT(g_device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		g_cmdAllocator.Get(), // 相关的命令分配器
		nullptr, // 管线状态
		IID_PPV_ARGS(g_cmdList.GetAddressOf())
	));
	g_cmdList->Close();

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
		g_cmdQueue.Get(),
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

	FlushCmdQueue();
	CHECK_HRESULT(g_cmdList->Reset(g_cmdAllocator.Get(), nullptr));
	// 将深度/模板缓冲区状态转为深度缓冲区
	g_cmdList->ResourceBarrier(1,
		&RvalueToLvalue(CD3DX12_RESOURCE_BARRIER::Transition(
			g_depthStencilBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_DEPTH_WRITE
		))
	);

	// 发送命令
	CHECK_HRESULT(g_cmdList->Close());
	ID3D12CommandList* cmdsLists[] = { g_cmdList.Get() };
	g_cmdQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
	FlushCmdQueue();

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

bool AppInit() {

	CHECK_HRESULT(g_cmdList->Reset(g_cmdAllocator.Get(), nullptr));

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

	CHECK_HRESULT(g_cmdList->Close());
	ID3D12CommandList* cmdsLists[] = { g_cmdList.Get() };
	g_cmdQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCmdQueue();

	return true;
}

void Update() {

}

void Render() {
	
	CHECK_HRESULT(g_cmdAllocator->Reset());
	CHECK_HRESULT(g_cmdList->Reset(g_cmdAllocator.Get(), nullptr));

	CD3DX12_RESOURCE_BARRIER barrier;
	// 清除 Render Target
	// 通知 GPU 资源的状态即将改变
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		g_swapChainBuffers[g_currBackBufferIndex].Get(), // 目标资源：当前后台缓冲区
		D3D12_RESOURCE_STATE_PRESENT, // 当前状态：呈现到屏幕
		D3D12_RESOURCE_STATE_RENDER_TARGET // 目标状态：作为渲染目标使用
	);
	g_cmdList->ResourceBarrier(1, &barrier);

	// 设置渲染目标
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(
		g_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		g_currBackBufferIndex,
		g_rtvDescriptorSize
	);
	g_cmdList->OMSetRenderTargets(1, &rtv, true, nullptr);

	// 清除渲染目标
	FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
	g_cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
	
	// 通知 GPU 资源的状态即将改变
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		g_swapChainBuffers[g_currBackBufferIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);
	g_cmdList->ResourceBarrier(1, &barrier);

	// 提交命令
	CHECK_HRESULT(g_cmdList->Close());
	ID3D12CommandList* cmdsLists[] = { g_cmdList.Get() };
	g_cmdQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// 呈现当前的后台缓冲区
	CHECK_HRESULT(g_swapChain->Present(1, 0));

	// 交换前后缓存
	g_currBackBufferIndex = (g_currBackBufferIndex + 1) % k_swapChainBufferCount;

	// 等待命令完成
	FlushCmdQueue();
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
