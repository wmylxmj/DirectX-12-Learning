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

#include <DirectXColors.h>
// -----------------------------------------
#include <string>
#include <cassert>
#include <vector>
#include <array>

// �Լ���ͷ�ļ�
#include "D3D12/Helper.h"


struct ObjectConstants
{
	DirectX::XMFLOAT4X4 WorldViewProj;
};

// ������
struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT4 Color;
};


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
int g_currBackBufferIndex = 0;

Microsoft::WRL::ComPtr<ID3D12Resource> g_depthStencilBuffer;
Microsoft::WRL::ComPtr<ID3D12Resource> g_constantBuffer;


Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_rtvDescriptorHeap;
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_dsvDescriptorHeap;
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_cbvDescriptorHeap;

UINT g_rtvDescriptorSize;

BYTE* g_cbMappedData = nullptr;

Microsoft::WRL::ComPtr<ID3D12RootSignature> g_rootSignature;


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

	// ����������� To do: ��ͬ���͵�������зֿ�
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	CHECK_HRESULT(g_device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&g_cmdQueue)));

	// ������������� To do: ��ͬ���͵�����������ֿ�
	CHECK_HRESULT(g_device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(g_cmdAllocator.GetAddressOf())
	));

	// ���������б� To do: ��ͬ���͵������б�ֿ�
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
		// ��ý������еĺ�̨������
		CHECK_HRESULT(g_swapChain->GetBuffer(i, IID_PPV_ARGS(&g_swapChainBuffers[i])));
		g_device->CreateRenderTargetView(g_swapChainBuffers[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, g_rtvDescriptorSize);
	}

	// �������/ģ�建����������ͼ
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
	// �Ż����ֵ
	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;
	// �������/ģ�建����
	CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CHECK_HRESULT(g_device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&dsvBufferDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&clearValue,
		IID_PPV_ARGS(g_depthStencilBuffer.GetAddressOf())
	));
	// ������ͼ
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.Texture2D.MipSlice = 0;
	g_device->CreateDepthStencilView(g_depthStencilBuffer.Get(), &dsvDesc, g_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	FlushCmdQueue();
	CHECK_HRESULT(g_cmdList->Reset(g_cmdAllocator.Get(), nullptr));
	// �����/ģ�建����״̬תΪ��Ȼ�����
	g_cmdList->ResourceBarrier(1,
		&RvalueToLvalue(CD3DX12_RESOURCE_BARRIER::Transition(
			g_depthStencilBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_DEPTH_WRITE
		))
	);

	// ��������
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

	// ����ʵ�ʵ�Ĭ�ϻ�������Դ
	CHECK_HRESULT(device->CreateCommittedResource(
		&RvalueToLvalue(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
		D3D12_HEAP_FLAG_NONE,
		&RvalueToLvalue(CD3DX12_RESOURCE_DESC::Buffer(byteSize)), // Resource Desc
		D3D12_RESOURCE_STATE_COMMON,
		nullptr, // Clear Value
		IID_PPV_ARGS(defaultBuffer.GetAddressOf())
	));

	// �����н��ϴ��ѣ���CPU�ڴ�����ͨ���ϴ��Ѹ��Ƶ�Ĭ�϶�
	CHECK_HRESULT(device->CreateCommittedResource(
		&RvalueToLvalue(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
		D3D12_HEAP_FLAG_NONE,
		&RvalueToLvalue(CD3DX12_RESOURCE_DESC::Buffer(byteSize)),
		D3D12_RESOURCE_STATE_GENERIC_READ, // ������Դ�� CPU �� GPU ֮����ж�ȡ
		nullptr,
		IID_PPV_ARGS(uploadBuffer.GetAddressOf())
	));

	// ������Ҫ���Ƶ�Ĭ�϶ѵ�����
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;
	subResourceData.RowPitch = byteSize; // ���ڻ��������ò���Ϊ�ֽ���
	subResourceData.SlicePitch = byteSize; // ���ڻ��������ò���Ϊ�ֽ���

	// CPU �ڴ������ϴ����н��ϴ��ѣ���ͨ���ϴ����ϴ���Ĭ�϶�
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

	// ������ɺ�����ͷ��ϴ���

	return defaultBuffer;
}

UINT CalcConstantBufferByteSize(UINT byteSize)
{
	// byteSize + 255���������256�ı��������ϵ���
	// & ~255������� 8 λ��ȷ������� 256 �ֽڶ����
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
		entrypoint.c_str(), // ��ڵ�
		target.c_str(), // ��ɫ�����ͺͰ汾
		compileFlags, // �����־ѡ��
		0, // �߼�����ѡ��
		&byteCode, 
		&errors
	);

	if (errors != nullptr)
		OutputDebugStringA((char*)errors->GetBufferPointer());

	CHECK_HRESULT(hr);

	return byteCode;
}

bool AppInit() {

	CHECK_HRESULT(g_cmdList->Reset(g_cmdAllocator.Get(), nullptr));

	// ����������������������
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	CHECK_HRESULT(g_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&g_cbvDescriptorHeap)));

	// ��������������
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

	// ����������������ͼ
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = g_constantBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = CalcConstantBufferByteSize(sizeof(ObjectConstants));
	g_device->CreateConstantBufferView(
		&cbvDesc,
		g_cbvDescriptorHeap->GetCPUDescriptorHandleForHeapStart()
	);

	// ������ǩ��
	// ����������
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];

	// ������������
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 
		1, // ����������
		0 // ��׼��ɫ���Ĵ���
	);
	slotRootParameter[0].InitAsDescriptorTable(
		1, // ��������������
		&cbvTable // ���������������ָ��
	);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		1, // ����������
		slotRootParameter, // ָ�������ָ��
		0, 
		nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	// ����������һ����λ�ĸ�ǩ��
	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(
		&rootSigDesc, 
		D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), 
		errorBlob.GetAddressOf()
	);
	if (errorBlob != nullptr)
	{
		MessageBoxA(0, (char*)errorBlob->GetBufferPointer(), "Failed To Serialize RootSignature", 0);
	}
	CHECK_HRESULT(hr);

	CHECK_HRESULT(g_device->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&g_rootSignature)
	));

	// ������ɫ��
	Microsoft::WRL::ComPtr<ID3DBlob> vsByteCode = CompileShader(L"Source\\Shaders\\color.hlsl", nullptr, "VS", "vs_5_1");
	Microsoft::WRL::ComPtr<ID3DBlob> psByteCode = CompileShader(L"Source\\Shaders\\color.hlsl", nullptr, "PS", "ps_5_1");

	// ���㲼��
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// ������Ӽ�����
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
		g_cmdList.Get(),
		vertices.data(),
		vbByteSize,
		VertexBufferUploader
	);

	g_indexBufferGPU = CreateDefaultBuffer(
		g_device.Get(),
		g_cmdList.Get(), 
		indices.data(), 
		ibByteSize, 
		IndexBufferUploader
	);

	// �������㻺������ͼ
	g_vbv.BufferLocation = g_vertexBufferGPU->GetGPUVirtualAddress();
	g_vbv.StrideInBytes = sizeof(Vertex);
	g_vbv.SizeInBytes = vbByteSize;

	// ����������������ͼ
	g_ibv.BufferLocation = g_indexBufferGPU->GetGPUVirtualAddress();
	g_ibv.Format = DXGI_FORMAT_R16_UINT;
	g_ibv.SizeInBytes = ibByteSize;
 
	// ���ù���״̬
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { inputLayout.data(), (UINT)inputLayout.size() }; // ���붥�㲼��
	psoDesc.pRootSignature = g_rootSignature.Get(); // ��ǩ��
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
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // ��դ��״̬
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT); // ���״̬
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // ���/ģ��״̬���������/ģ����ԣ�
	psoDesc.SampleMask = UINT_MAX; // ���ò���������λ��Ϊ0��
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // ͼԪ����������
	psoDesc.NumRenderTargets = 1; // ��ȾĿ������
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // ��ȾĿ��ĸ�ʽ
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT; // ���/ģ�建�����ĸ�ʽ
	CHECK_HRESULT(g_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_pso)));

	CHECK_HRESULT(g_cmdList->Close());
	ID3D12CommandList* cmdsLists[] = { g_cmdList.Get() };
	g_cmdQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCmdQueue();

	return true;
}

void Update() {
	float mTheta = 1.5f * DirectX::XM_PI;
	float mPhi = DirectX::XM_PIDIV4;
	float mRadius = 5.0f;

	float x = mRadius * sinf(mPhi) * cosf(mTheta);
	float z = mRadius * sinf(mPhi) * sinf(mTheta);
	float y = mRadius * cosf(mPhi);

	DirectX::XMVECTOR pos = DirectX::XMVectorSet(x, y, z, 1.0f);
	DirectX::XMVECTOR target = DirectX::XMVectorZero();
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, target, up);

	DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX proj = DirectX::XMMatrixIdentity();
	
	DirectX::XMMATRIX worldViewProj = world * view * proj;

	ObjectConstants objConstants;
	XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));

	memcpy(&g_cbMappedData[0], &objConstants.WorldViewProj, sizeof(ObjectConstants));
}

void Render() {
	
	CHECK_HRESULT(g_cmdAllocator->Reset());
	CHECK_HRESULT(g_cmdList->Reset(g_cmdAllocator.Get(), nullptr));

	CD3DX12_RESOURCE_BARRIER barrier;
	// ��� Render Target
	// ֪ͨ GPU ��Դ��״̬�����ı�
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		g_swapChainBuffers[g_currBackBufferIndex].Get(), // Ŀ����Դ����ǰ��̨������
		D3D12_RESOURCE_STATE_PRESENT, // ��ǰ״̬�����ֵ���Ļ
		D3D12_RESOURCE_STATE_RENDER_TARGET // Ŀ��״̬����Ϊ��ȾĿ��ʹ��
	);
	g_cmdList->ResourceBarrier(1, &barrier);

	// ������ȾĿ��
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(
		g_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		g_currBackBufferIndex,
		g_rtvDescriptorSize
	);
	g_cmdList->OMSetRenderTargets(1, &rtv, true, nullptr);

	// ���òü�����
	D3D12_VIEWPORT viewport;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = (FLOAT)g_viewportWidth;
	viewport.Height = (FLOAT)g_viewportHeight;
	viewport.MinDepth = D3D12_MIN_DEPTH;
	viewport.MaxDepth = D3D12_MAX_DEPTH;
	g_cmdList->RSSetViewports(1, &viewport);

	// �����ȾĿ��
	FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
	g_cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
	g_cmdList->ClearDepthStencilView(g_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// ��ʼ��Ⱦ
	ID3D12DescriptorHeap* descriptorHeaps[] = { g_cbvDescriptorHeap.Get() };
	g_cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	g_cmdList->SetGraphicsRootSignature(g_rootSignature.Get());

	g_cmdList->IASetVertexBuffers(0, 1, &g_vbv);
	g_cmdList->IASetIndexBuffer(&g_ibv);
	g_cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	g_cmdList->SetGraphicsRootDescriptorTable(0, g_cbvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	
	g_cmdList->DrawIndexedInstanced(
		g_indicesCount, // һ��ʵ������������
		1, // INSTANCE COUNT
		0, // START INDEX LOCATION
		0, // BASE VERTEX LOCATION
		0 // START INSTANCE LOCATION
	);

	// ֪ͨ GPU ��Դ��״̬�����ı�
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		g_swapChainBuffers[g_currBackBufferIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);
	g_cmdList->ResourceBarrier(1, &barrier);

	// �ύ����
	CHECK_HRESULT(g_cmdList->Close());
	ID3D12CommandList* cmdsLists[] = { g_cmdList.Get() };
	g_cmdQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// ���ֵ�ǰ�ĺ�̨������
	CHECK_HRESULT(g_swapChain->Present(0, 0));

	// ����ǰ�󻺴�
	g_currBackBufferIndex = (g_currBackBufferIndex + 1) % k_swapChainBufferCount;

	// �ȴ��������
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
