#include "Windows.h"
#include "WindowsX.h"

#include <wrl.h> // ���� Microsoft ������ָ��
#include <comdef.h> // ���ڴ��� HRESULT ����

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

// �Լ���ͷ�ļ�
#include "D3D12/Helper.h"

// ----------------ȫ�ֱ���------------------
uint32_t g_viewportWidth = 640;
uint32_t g_viewportHeight = 480;

HWND g_hMainWnd;
Microsoft::WRL::ComPtr<ID3D12Device2> g_device;

// ����ͬ��
Microsoft::WRL::ComPtr<ID3D12Fence> g_fence;
uint64_t g_fenceValue = 0;

Microsoft::WRL::ComPtr<ID3D12CommandQueue> g_cmdQueue;
Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> g_cmdList;
// ���̨�������������
Microsoft::WRL::ComPtr<ID3D12CommandAllocator> g_cmdAllocator;
Microsoft::WRL::ComPtr<IDXGISwapChain> g_swapChain;

// ������������
const int k_swapChainBufferCount = 2;
Microsoft::WRL::ComPtr<ID3D12Resource> g_swapChainBuffers[k_swapChainBufferCount];

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_rtvDescriptorHeap;
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_dsvDescriptorHeap;

UINT g_rtvDescriptorSize;

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

bool InitMainWindow(HINSTANCE hInstance) {

	// ����������
	WNDCLASSEX wc;
	wc.cbSize = sizeof(wc); // �ṹ�Ĵ�С
	wc.style = CS_HREDRAW | CS_VREDRAW; // ����ʽ
	wc.lpfnWndProc = MainWndProc; // ���ڴ�����
	wc.cbClsExtra = 0; // ����Ķ���������
	wc.cbWndExtra = 0; // �����͵�ÿ�����ڵĶ���������
	wc.hInstance = hInstance; // Ӧ�ó���ʵ���ľ��
	wc.hIcon = nullptr; // 32x32��ͼ��
	wc.hCursor = nullptr; // ���
	wc.hbrBackground = nullptr; // ������ˢ�����ô�����ɫ
	wc.lpszMenuName = nullptr; // �˵�����
	wc.lpszClassName = L"MainWnd"; // ������
	wc.hIconSm = nullptr; // 16x16Сͼ��

	// ע�ᴰ����
	if (!RegisterClassEx(&wc))
	{
		MessageBox(0, L"Register Window Class Failed.", 0, 0);
		return false;
	}

	// ��������ʵ��
	g_hMainWnd = CreateWindowEx(
		0, // ��չ������ʽ
		wc.lpszClassName, // ����������
		L"Title", // ���ڱ���
		WS_OVERLAPPEDWINDOW, // ������ʽ����
		CW_USEDEFAULT, CW_USEDEFAULT, g_viewportWidth, g_viewportHeight, // ���Ͻ����꣬���
		0, 0, hInstance, 0 // �����ھ�����˵������Ӧ�ó���ʵ�������һ��ָ�򴰿ڴ������ݵ�ָ��
	);

	if (!g_hMainWnd)
	{
		MessageBox(0, L"Create Window Failed.", 0, 0);
		return false;
	}

	// ��ʾ�����´���
	ShowWindow(g_hMainWnd, SW_SHOW);
	UpdateWindow(g_hMainWnd);

	return true;
}

void FlushCmdQueue() {
	g_fenceValue++;
	// ����ͬ���ź�
	CHECK_HRESULT(g_cmdQueue->Signal(g_fence.Get(), g_fenceValue));
	// �ȴ� GPU �������
	if (g_fence->GetCompletedValue() < g_fenceValue) {
		HANDLE eventHandle = CreateEventEx(nullptr, L"Fence Event", false, EVENT_ALL_ACCESS);
		assert(eventHandle != nullptr && "Create Fence Event Failed.");
		CHECK_HRESULT(g_fence->SetEventOnCompletion(g_fenceValue, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}


bool InitDirect3D() {

	// �������Բ�
#if defined(_DEBUG)
	Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
	CHECK_HRESULT(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
#endif

	// ���ڴ����豸�Ĺ���
	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
	CHECK_HRESULT(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

	// ����D3D�豸 To do: ����������ʱʹ��WARP�豸
	CHECK_HRESULT(D3D12CreateDevice(
		nullptr, // Ĭ��������
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&g_device)
	));

	// ����Χ��������CPU��GPUͬ��
	CHECK_HRESULT(g_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)));

	// �����������
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	CHECK_HRESULT(g_device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&g_cmdQueue)));

	// �������������
	CHECK_HRESULT(g_device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(g_cmdAllocator.GetAddressOf())
	));

	// ���������б�
	CHECK_HRESULT(g_device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		g_cmdAllocator.Get(), // ��ص����������
		nullptr, // ����״̬
		IID_PPV_ARGS(g_cmdList.GetAddressOf())
	));
	g_cmdList->Close();

	// ����������
	g_swapChain.Reset();
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferDesc.Width = g_viewportWidth;
	swapChainDesc.BufferDesc.Height = g_viewportHeight;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.SampleDesc.Count = 1; // ��תģ���޷�ʹ�ö��ز���
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

	// ���� RTV �� DSV ��������
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

	// ���� RTV
	g_rtvDescriptorSize = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(g_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	for (int i = 0; i < k_swapChainBufferCount; ++i) {
		CHECK_HRESULT(g_swapChain->GetBuffer(i, IID_PPV_ARGS(&g_swapChainBuffers[i])));
		g_device->CreateRenderTargetView(g_swapChainBuffers[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, g_rtvDescriptorSize);
	}

	return true;
}

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow
) {
	InitMainWindow(hInstance);
	InitDirect3D();
	OutputDebugStringA("Hello World!\n");
	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return 0;
}


