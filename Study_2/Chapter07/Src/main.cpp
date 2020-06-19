#include <Windows.h>
#include <tchar.h>

#ifdef _DEBUG
#include <iostream>
#endif

using namespace std;

#include <d3d12.h>
#include <dxgi1_6.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include <wrl.h>
using namespace Microsoft::WRL;

#include <vector>

#include <DirectXMath.h>
using namespace DirectX;

#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

#include <DirectXTex.h>
#pragma comment(lib, "DirectXTex.lib")

#include <d3dx12.h>

constexpr size_t pmdvertex_size = 38; // 1���_������̃T�C�Y

// @brief �R���\�[����ʂɃt�H�[�}�b�g�t���������\��
// @param format �t�H�[�}�b�g�i%d�Ƃ�%f�Ƃ��́j
// @param �ϒ�����
// @remarks ���̊֐��̓f�o�b�O�p�ł��B�f�o�b�O���ɂ������삵�܂���
void DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG

	va_list valist;
	va_start(valist, format);
	printf(format, valist);
	va_end(valist);

#endif
}

// �A���C�������g�ɂ��낦���T�C�Y��Ԃ�
// @param size ���̃T�C�Y
// @param alignment �A���C�������g�T�C�Y
// @return �A���C�������g�����낦���T�C�Y
size_t AlignmentedSize(size_t size, size_t alignment)
{
	if (size % alignment == 0) return size;

	return size + alignment - size % alignment;
}

void EnableDebugLayer()
{
	ID3D12Debug* debugLayer{ nullptr };
	auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));

	debugLayer->EnableDebugLayer(); // �f�o�b�O���C���[��L��������
	debugLayer->Release(); // �L����������C���^�[�t�F�C�X���������
}

const int window_width = 960;
const int window_height = 540;

// �ʓ|�����Ǐ����Ȃ���΂����Ȃ��֐�
HRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// �E�B���h�E���j�����ꂽ��Ă΂��
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0); // OS�ɑ΂��āu�������̃A�v���͏I���v�Ɠ`����
		return 0;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam); // ����̏������s��
}

#ifdef _DEBUG

int main()
{

#else

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{

#endif

	DebugOutputFormatString("Show window test.");

	// �E�B���h�E�N���X�̐������o�^
	WNDCLASSEX w{};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure; // �R�[���o�b�N�֐��̎w��
	w.lpszClassName = _T("DX12Sample");       // �A�v���P�[�V�����N���X���i�K���ł悢�j
	w.hInstance = GetModuleHandle(nullptr);   // �n���h���̎擾

	RegisterClassEx(&w); // �A�v���P�[�V�����N���X�i�E�B���h�E�N���X�̎w���OS�ɓ`����j

	RECT wrc = { 0, 0, window_width, window_height }; // �E�B���h�E�T�C�Y�����߂�

	// �֐����g���ăE�B���h�E�̃T�C�Y��␳����
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// �E�B���h�E�I�u�W�F�N�g�̐���
	HWND hwnd = CreateWindow(
		w.lpszClassName,      // �N���X���w��
		_T("DX12�e�X�g"),	  // �^�C�g���o�[�̕���
		WS_OVERLAPPEDWINDOW,  // �^�C�g���o�[�Ƌ��E������E�B���h�E
		CW_USEDEFAULT,		  // �\�� X ���W��OS�ɂ��C��
		CW_USEDEFAULT,		  // �\�� Y ���W��OS�ɂ��C��
		wrc.right - wrc.left, // �E�B���h�E��
		wrc.bottom - wrc.top, // �E�B���h�E��
		nullptr,			  // �e�E�B���h�E�n���h��
		nullptr,			  // ���j���[�n���h��
		w.hInstance,		  // �Ăяo���A�v���P�[�V�����n���h��
		nullptr);			  // �ǉ��p�����[�^�[

	// �E�B���h�E�\��
	ShowWindow(hwnd, SW_SHOW);

	// ==========DirectX12�֌W==========

#ifdef _DEBUG

	// �f�o�b�O���C���[���I����
	EnableDebugLayer();

#endif

	ComPtr<ID3D12Device>    _dev{ nullptr };
	ComPtr<IDXGIFactory6>   _dxgiFactory{ nullptr };
	ComPtr<IDXGISwapChain4> _swapChain{ nullptr };

	// Direct3D�f�o�C�X�̏�����
	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	D3D_FEATURE_LEVEL featureLevel;

	for (auto lv : levels)
	{
		if (D3D12CreateDevice(nullptr, lv, IID_PPV_ARGS(_dev.ReleaseAndGetAddressOf())))
		{
			featureLevel = lv;
			break; // �����\�ȃo�[�W���������������烋�[�v��ł��؂�
		}
	}

	// �t�@�N�g���[�̍쐬
#ifdef _DEBUG

	// DXGI�֌W�̃G���[���b�Z�[�W��\���ł���
	auto result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf()));

#else

	auto result = CreateDXGIFactory1(IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf()));

#endif
	ComPtr<ID3D12CommandAllocator>    _cmdAllocator{ nullptr };
	ComPtr<ID3D12GraphicsCommandList> _cmdList{ nullptr };
	ComPtr<ID3D12CommandQueue>        _cmdQueue{ nullptr };

	// �R�}���h�A���P�[�^�[�쐬
	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(_cmdAllocator.ReleaseAndGetAddressOf()));

	// �R�}���h���X�g�쐬
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator.Get(), nullptr, IID_PPV_ARGS(_cmdList.ReleaseAndGetAddressOf()));

	// �R�}���h�L���[�쐬
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc{};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; // �t���O�Ȃ�
	cmdQueueDesc.NodeMask = 0; // �A�_�v�^�[��1�����g��Ȃ��Ƃ���0�ł悢
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL; // �v���C�I���e�B�͓��Ɏw��Ȃ�
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // �R�}���h���X�g�ƍ��킹��
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(_cmdQueue.ReleaseAndGetAddressOf()));

	// �X���b�v�`�F�[���쐬
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = window_width;
	swapChainDesc.Height = window_height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH; // �o�b�N�o�b�t�@�[�͐L�яk�݉\
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // �t���b�v��͑��₩�ɔj��
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED; // ���Ɏw��Ȃ�
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // �E�B���h�E�̃t���X�N���[���؂�ւ��\
	result = _dxgiFactory->CreateSwapChainForHwnd(_cmdQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, (IDXGISwapChain1**)_swapChain.ReleaseAndGetAddressOf());

	// �t�F���X�쐬
	ComPtr<ID3D12Fence> _fence{ nullptr };
	UINT64 _fenceVal = 0;
	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_fence.ReleaseAndGetAddressOf()));

	// RTV�쐬�i�܂��q�[�v������ė̈���m�ۂ��Ă���RTV���쐬�j
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // �����_�[�^�[�Q�b�g�r���[�Ȃ̂�RTV
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2; // �\����2��
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // ���Ɏw��Ȃ��i�V�F�[�_�[����ǂ������邩�B����̓V�F�[�_�[���猩�邱�Ƃ͂Ȃ��̂Ŏw��Ȃ��j

	ComPtr<ID3D12DescriptorHeap> _rtvHeaps{ nullptr };
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_rtvHeaps.ReleaseAndGetAddressOf()));

	DXGI_SWAP_CHAIN_DESC swcDesc{};
	result = _swapChain->GetDesc(&swcDesc);

	std::vector<ComPtr<ID3D12Resource>> _backBuffers(swcDesc.BufferCount);

	// �r���[�̃A�h���X
	auto handle = _rtvHeaps->GetCPUDescriptorHandleForHeapStart();

	// SRGB�����_�[�^�[�Q�b�g�r���[�ݒ�
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // �K���}�␳����isRGB�j
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (int i = 0; i < swcDesc.BufferCount; ++i)
	{
		// �X���b�v�`�F�[���쐬���Ɋm�ۂ����o�b�t�@���擾
		result = _swapChain->GetBuffer(i, IID_PPV_ARGS(_backBuffers[i].ReleaseAndGetAddressOf()));

		// �擾�����o�b�t�@�̂Ȃ���RTV���쐬����i���Ă������ꂩ�ȁH�j
		_dev->CreateRenderTargetView(_backBuffers[i].Get(), &rtvDesc, handle);

		// ���̂܂܂��Ɠ������������Ă��܂��̂ŃA�h���X�����炷
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}


	// �[�x�o�b�t�@�[�쐬
	D3D12_RESOURCE_DESC depthResDesc{};
	depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2�����̃e�N�X�`���f�[�^
	depthResDesc.Width = window_width;   // ���ƍ����̓����_�[�^�[�Q�b�g�Ɠ���
	depthResDesc.Height = window_height;
	depthResDesc.DepthOrArraySize = 1;   // �e�N�X�`���z��ł��A3D�e�N�X�`���ł��Ȃ�
	depthResDesc.Format = DXGI_FORMAT_D32_FLOAT; // �[�x�l�������ݗp�t�H�[�}�b�g
	depthResDesc.SampleDesc.Count = 1;
	depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // �f�v�X�X�e���V���Ƃ��Ďg�p

	// �[�x�l�p�q�[�v�v���p�e�B
	D3D12_HEAP_PROPERTIES depthHeapProp{};
	depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT; // DEFAULT�Ȃ̂ł��Ƃ�UNKNOWN�ł悢
	depthHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	
	// ���̃N���A�o�����[���d�v�ȈӖ�������
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f; // �[�� 1.0f(�ő�l�j�ŃN���A
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT; // 32�r�b�gfloat�l�Ƃ��ăN���A

	ComPtr<ID3D12Resource> depthBuffer{ nullptr };
	result = _dev->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, // �[�x�l��������
		&depthClearValue,
		IID_PPV_ARGS(depthBuffer.ReleaseAndGetAddressOf()));

	// �[�x�̂��߂̃f�B�X�N���v�^�q�[�v�쐬
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{}; // �[�x�Ɏg�����Ƃ��킩��΂悢
	dsvHeapDesc.NumDescriptors = 1; // �[�x�r���[��1��
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV; // �f�v�X�X�e���V���r���[�Ƃ��Ďg��
	
	ComPtr<ID3D12DescriptorHeap> dsvHeap{ nullptr };
	result = _dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(dsvHeap.ReleaseAndGetAddressOf()));

	// �[�x�r���[�쐬
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // �[�x�l��32�r�b�g�g�p
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2D�e�N�X�`��
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE; // �t���O�͓��ɂȂ�
	_dev->CreateDepthStencilView(depthBuffer.Get(), &dsvDesc, dsvHeap->GetCPUDescriptorHandleForHeapStart());

	// �J�������̐ݒ�
	struct MatricesData
	{
		XMMATRIX world; // ���f���{�̂���]��������ړ��������肷��s��
		XMMATRIX viewproj; // �r���[�ƃv���W�F�N�V���������s��
	};

	XMMATRIX worldMat = XMMatrixIdentity();
	XMFLOAT3 eye{ 0.0f, 10.0f, -15.0f };
	XMFLOAT3 target{ 0.0f, 10.0f, 0.0f };
	XMFLOAT3 up{ 0.0f, 1.0f, 0.0f };

	worldMat = XMMatrixRotationY(XM_PIDIV4);
	auto viewMat = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
	auto projMat = XMMatrixPerspectiveFovLH(
		XM_PIDIV2,
		static_cast<float>(window_width) / static_cast<float>(window_height),
		1.0f,
		100.0f);

	// �萔�o�b�t�@�쐬
	ComPtr<ID3D12Resource> constBuff{ nullptr };

	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(MatricesData) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(constBuff.ReleaseAndGetAddressOf()));

	MatricesData* mapMat{ nullptr }; // �}�b�v��������|�C���^�[
	result = constBuff->Map(0, nullptr, (void**)&mapMat); // �}�b�v
	mapMat->world = worldMat; // �s��̓��e���R�s�[
	mapMat->viewproj = viewMat * projMat;

		// PMD�f�[�^
	struct PMDHeader
	{
		float version;       // ��F00 00 80 3F == 1.00
		char model_name[20]; // ���f����
		char comment[256];   // ���f���R�����g
	};

	// PMD���_�f�[�^
	struct PMDVertex
	{
		XMFLOAT3 pos;             // ���_���W
		XMFLOAT3 normal;          // �@���x�N�g��
		XMFLOAT2 uv;              // uv���W
		unsigned short boneNo[2]; // �{�[���ԍ�
		unsigned char boneWeight; // �{�[���e���x
		unsigned char edgeFlag;   // �֊s���t���O
	};

	PMDHeader pmdHeader;
	char signature[3]{}; // pmd�t�@�C���̐擪3�o�C�g���uPmd�v�ƂȂ��Ă��邽�߁A����͍\���̂Ɋ܂߂Ȃ�
	FILE* fp;
	unsigned int vertNum; // ���_��
	std::vector<unsigned char> vertices; // �o�b�t�@�̊m��
	unsigned int indicesNum; // �C���f�b�N�X��
	std::vector<unsigned short> indices;
	
	fopen_s(&fp, "Model/�����~�N.pmd", "rb");
	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdHeader, sizeof(pmdHeader), 1, fp);

	fread(&vertNum, sizeof(vertNum), 1, fp);
	vertices.resize(vertNum * pmdvertex_size);
	fread(vertices.data(), vertices.size(), 1, fp);
	
	fread(&indicesNum, sizeof(indicesNum), 1, fp);
	indices.resize(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	fclose(fp);

	ComPtr<ID3D12Resource> vertBuff{ nullptr };

	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertices.size()), // �T�C�Y�ύX
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(vertBuff.ReleaseAndGetAddressOf()));

	unsigned char* vertMap{ nullptr };
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(vertices), std::end(vertices), vertMap);
	vertBuff->Unmap(0, nullptr);

	D3D12_VERTEX_BUFFER_VIEW vbView{};
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress(); // �o�b�t�@�̉��z�A�h���X
	vbView.SizeInBytes = vertices.size(); // �S�o�C�g��
	vbView.StrideInBytes = pmdvertex_size; // 1���_������̃o�C�g��

	ComPtr<ID3D12Resource> idxBuff{ nullptr };

	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(idxBuff.ReleaseAndGetAddressOf()));

	D3D12_INDEX_BUFFER_VIEW ibView{};
	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = indices.size() * sizeof(indices[0]);

	// ������o�b�t�@�[�ɃC���f�b�N�X�f�[�^���R�s�[
	unsigned short* mappedIdx{ nullptr };
	result = idxBuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(std::begin(indices), std::end(indices), mappedIdx);
	idxBuff->Unmap(0, nullptr);

	ComPtr<ID3DBlob> vsBlob{ nullptr };
	ComPtr<ID3DBlob> psBlob{ nullptr };
	ComPtr<ID3DBlob> errorBlob{ nullptr };

	// ���_�V�F�[�_�[
	result = D3DCompileFromFile(
		L"Shader/BasicVertexShader.hlsl", // �V�F�[�_�[��
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // �C���N���[�h�̓f�t�H���g
		"BasicVS", // �֐��́uBasicVS�v
		"vs_5_0", // �ΏۃV�F�[�_�[�́uvs_5_0�v
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // �f�o�b�O�p�y�эœK���͂Ȃ�
		0,
		vsBlob.ReleaseAndGetAddressOf(),
		errorBlob.ReleaseAndGetAddressOf()); // �G���[���̓��b�Z�[�W������

	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA("�t�@�C������������܂���");
			return 0; // exit()�ȂǂɓK�X�u������������悢
		}
		else
		{
			std::string errStr;
			errStr.resize(errorBlob->GetBufferSize());

			std::copy_n(
				(char*)errorBlob->GetBufferPointer(),
				errorBlob->GetBufferSize(),
				errStr.begin());
			errStr += "\n";
			::OutputDebugStringA(errStr.c_str());
		}
	}

	// �s�N�Z���V�F�[�_�[
	result = D3DCompileFromFile(
		L"Shader/BasicPixelShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicPS",
		"ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		psBlob.ReleaseAndGetAddressOf(),
		errorBlob.ReleaseAndGetAddressOf());

	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA("�t�@�C������������܂���");
			return 0; // exit()�ȂǂɓK�X�u������������悢
		}
		else
		{
			std::string errStr;
			errStr.resize(errorBlob->GetBufferSize());

			std::copy_n(
				(char*)errorBlob->GetBufferPointer(),
				errorBlob->GetBufferSize(),
				errStr.begin());
			errStr += "\n";
			::OutputDebugStringA(errStr.c_str());
		}
	}

	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // ���W
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // �@��
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // uv
		{ "BONE_NO",  0, DXGI_FORMAT_R16G16_UINT,     0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // �{�[���ԍ�
		{ "WEIGHT",   0, DXGI_FORMAT_R8_UINT,         0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // �E�F�C�g
	};

	ComPtr<ID3D12DescriptorHeap> basicDescHeap{ nullptr };
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // �V�F�[�_�[���猩����悤��
	descHeapDesc.NodeMask = 0; // �}�X�N��0
	descHeapDesc.NumDescriptors = 2; // SRV, CBV
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; // �V�F�[�_�[���\�[�X�r���[�p
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(basicDescHeap.ReleaseAndGetAddressOf()));

	auto basicHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = constBuff->GetDesc().Width;

	// �萔�o�b�t�@�r���[�쐬
	_dev->CreateConstantBufferView(
		&cbvDesc,
		basicHeapHandle);

	// �����W�̍쐬
	D3D12_DESCRIPTOR_RANGE range{};

	// �萔�p
	range.NumDescriptors = 1;
	range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	range.BaseShaderRegister = 0;
	range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ���[�g�p�����[�^�[�쐬
	D3D12_ROOT_PARAMETER rootParam{};
	rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // �V�F�[�_�[���猩����
	rootParam.DescriptorTable.pDescriptorRanges = &range; // �f�B�X�N���v�^�����W�̃A�h���X
	rootParam.DescriptorTable.NumDescriptorRanges = 1; // �����W��

	// �T���v���[�쐬
	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // �������̌J��Ԃ�
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // �c�����̌J��Ԃ�
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // ���s���̌J��Ԃ�
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK; // �{�[�_�[�͍�
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // ���`���
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX; // �~�b�v�}�b�v�ő�l
	samplerDesc.MinLOD = 0.0f; // �~�b�v�}�b�v�ŏ��l
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // �s�N�Z���V�F�[�_�[���猩����
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // ���T���v�����O���Ȃ�

	// ���[�g�V�O�l�`���쐬
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.pParameters = &rootParam; // ���[�g�p�����[�^�[�̐擪�A�h���X
	rootSignatureDesc.NumParameters = 1; // ���[�g�p�����[�^�[��
	rootSignatureDesc.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.NumStaticSamplers = 1;

	ID3DBlob* rootSigBlob{ nullptr };
	result = D3D12SerializeRootSignature(
		&rootSignatureDesc, // ���[�g�V�O�l�`���ݒ�
		D3D_ROOT_SIGNATURE_VERSION_1_0, // ���[�g�V�O�l�`���o�[�W����
		&rootSigBlob, // �V�F�[�_�[����������Ɠ���
		errorBlob.ReleaseAndGetAddressOf());

	ComPtr<ID3D12RootSignature> rootSignature{ nullptr };

	result = _dev->CreateRootSignature(
		0, // nodemask�B0�ł悢
		rootSigBlob->GetBufferPointer(), // �V�F�[�_�[�̂Ƃ��Ɠ���
		rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(rootSignature.ReleaseAndGetAddressOf()));

	rootSigBlob->Release(); // �s�v�ɂȂ����̂ŉ��

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline{};

	gpipeline.pRootSignature = rootSignature.Get();

	// ���_�V�F�[�_�[�ݒ�
	gpipeline.VS.pShaderBytecode = vsBlob->GetBufferPointer();
	gpipeline.VS.BytecodeLength = vsBlob->GetBufferSize();

	// �s�N�Z���V�F�[�_�[�ݒ�
	gpipeline.PS.pShaderBytecode = psBlob->GetBufferPointer();
	gpipeline.PS.BytecodeLength = psBlob->GetBufferSize();

	// �f�t�H���g�̃T���v���}�X�N��\���萔�i0xffffffff�j
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	// ���X�^���C�U�[�ݒ�
	gpipeline.RasterizerState.MultisampleEnable = false; // �܂��A���`�G�C���A�V���O���g��Ȃ����� false
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // �J�����O���Ȃ�
	gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; // ���g��h��Ԃ�
	gpipeline.RasterizerState.DepthClipEnable = true; // �[�x�����̃N���b�s���O�͗L����

	// �u�����h�X�e�[�g�ݒ�
	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc{};
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.LogicOpEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	gpipeline.BlendState.AlphaToCoverageEnable = false;
	gpipeline.BlendState.IndependentBlendEnable = false;
	gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

	gpipeline.InputLayout.pInputElementDescs = inputLayout; // ���C�A�E�g�擪�A�h���X
	gpipeline.InputLayout.NumElements = _countof(inputLayout); // ���C�A�E�g�z��̗v�f��

	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED; // �J�b�g�Ȃ�

	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // �O�p�`�ō\��

	// �����_�[�^�[�Q�b�g�ݒ�
	gpipeline.NumRenderTargets = 1;
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 0�`1�ɐ��K�����ꂽRGBA

	// �T���v�����w��
	gpipeline.SampleDesc.Count = 1; // �T���v�����O�� 1 �s�N�Z���ɂ� 1
	gpipeline.SampleDesc.Quality = 0; // �N�I���e�B�͍Œ�

	// �[�x�o�b�t�@�[����̐ݒ�
	gpipeline.DepthStencilState.DepthEnable = true; // �[�x�o�b�t�@���g��
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // �s�N�Z���`�掞�ɐ[�x�����ɏ�������
	gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // �����������̗p
	gpipeline.DepthStencilState.StencilEnable = false;
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	// �p�C�v���C���쐬
	ComPtr<ID3D12PipelineState> _pipelineState{ nullptr };
	result = _dev->CreateGraphicsPipelineState(
		&gpipeline,
		IID_PPV_ARGS(_pipelineState.ReleaseAndGetAddressOf()));

	// �r���[�|�[�g
	D3D12_VIEWPORT viewport{};
	viewport.Width = window_width;   // �o�͐�̕��i�s�N�Z�����j
	viewport.Height = window_height; // �o�͐�̍����i�s�N�Z�����j
	viewport.TopLeftX = 0; // �o�͐�̍�����W X
	viewport.TopLeftY = 0; // �o�͐�̍�����W Y
	viewport.MaxDepth = 1.0f; // �[�x�ő�l
	viewport.MinDepth = 0.0f; // �[�x�ŏ��l

	// �V�U�[��`
	D3D12_RECT scissorRect{};
	scissorRect.top = 0;                // �؂蔲������W
	scissorRect.left = 0;               // �؂蔲�������W
	scissorRect.right = window_width;   // �؂蔲���E���W
	scissorRect.bottom = window_height; // �؂蔲���������W

	float angle = 0;

	// ���[�v
	MSG msg{};
	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// �A�v���P�[�V�������I���Ƃ��� message �� WM_QUIT �ɂȂ�
		if (msg.message == WM_QUIT)
		{
			break;
		}

		angle += 0.01f;
		worldMat = XMMatrixRotationY(angle);
		mapMat->world = worldMat;

		// ���݂̃o�b�N�o�b�t�@�[�̃C���f�b�N�X���擾
		auto bbIdx = _swapChain->GetCurrentBackBufferIndex();

		_cmdList->ResourceBarrier(
			1, &CD3DX12_RESOURCE_BARRIER::Transition(
				_backBuffers[bbIdx].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		// �����_�[�^�[�Q�b�g�r���[�A�f�v�X�X�e���V���r���[���Z�b�g
		auto rtvH = _rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		auto dsvH = dsvHeap->GetCPUDescriptorHandleForHeapStart();

		_cmdList->OMSetRenderTargets(1, &rtvH, false, &dsvH);

		// �����_�[�^�[�Q�b�g�̃N���A
		float clearColor[4]{ 1.0f, 1.0f, 1.0f, 1.0f }; // �w�i�F�i���͉��F�j
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

		// �[�x�o�b�t�@�[�̃N���A
		_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		// �r���[�|�[�g���Z�b�g
		_cmdList->RSSetViewports(1, &viewport);

		// �V�U�[��`���Z�b�g
		_cmdList->RSSetScissorRects(1, &scissorRect);

		// �p�C�v���C�����Z�b�g
		_cmdList->SetPipelineState(_pipelineState.Get());

		// ���[�g�V�O�l�`�����Z�b�g
		_cmdList->SetGraphicsRootSignature(rootSignature.Get());

		// �v���~�e�B�u�g�|���W���Z�b�g
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// ���_�o�b�t�@���Z�b�g
		_cmdList->IASetVertexBuffers(0, 1, &vbView);

		// �C���f�b�N�X�o�b�t�@���Z�b�g
		_cmdList->IASetIndexBuffer(&ibView);

		// �f�B�X�N���v�^�q�[�v���Z�b�g
		_cmdList->SetDescriptorHeaps(1, basicDescHeap.GetAddressOf());

		// ���[�g�p�����[�^�[�ƃf�B�X�N���v�^�q�[�v�̊֘A�t��
		_cmdList->SetGraphicsRootDescriptorTable(
			0, // ���[�g�p�����[�^�[�C���f�b�N�X
			basicDescHeap->GetGPUDescriptorHandleForHeapStart()); // �q�[�v�A�h���X

		// �h���[�R�[��
		_cmdList->DrawIndexedInstanced(indicesNum, 1, 0, 0, 0);

		_cmdList->ResourceBarrier(
			1, &CD3DX12_RESOURCE_BARRIER::Transition(
				_backBuffers[bbIdx].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

		// �R�}���h���X�g���s
		_cmdList->Close();

		ID3D12CommandList* cmdLists[]{ _cmdList.Get() };
		_cmdQueue->ExecuteCommandLists(1, cmdLists);

		// ��������
		_cmdQueue->Signal(_fence.Get(), ++_fenceVal);
		while (_fence->GetCompletedValue() != _fenceVal)
		{
			// �C�x���g�n���h���̎擾
			auto event = CreateEvent(nullptr, false, false, nullptr);

			_fence->SetEventOnCompletion(_fenceVal, event);

			// �C�x���g����������܂ő҂�������iINFINITE�j
			WaitForSingleObject(event, INFINITE);

			// �C�x���g�n���h�������
			CloseHandle(event);
		}

		// �R�}���h���X�g�Ȃǂ̃N���A
		_cmdAllocator->Reset(); // �L���[�̃N���A
		_cmdList->Reset(_cmdAllocator.Get(), nullptr); // �ĂуR�}���h���X�g�����߂鏀��

		// �t���b�v
		_swapChain->Present(1, 0);
	}

	// �����N���X�͎g��Ȃ��̂œo�^��������
	UnregisterClass(w.lpszClassName, w.hInstance);

	return 0;
}
