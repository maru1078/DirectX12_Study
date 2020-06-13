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
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(worldMat) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(constBuff.ReleaseAndGetAddressOf()));

	XMMATRIX* mapMat{ nullptr }; // �}�b�v��������|�C���^�[
	result = constBuff->Map(0, nullptr, (void**)&mapMat); // �}�b�v
	*mapMat = worldMat; // �s��̓��e���R�s�[

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

	struct Vertex
	{
		XMFLOAT3 pos; // xyz���W
		XMFLOAT2 uv;  // uv���W
	};

	//// ���_�o�b�t�@�쐬
	//Vertex vertices[4] =
	//{
	//	{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } }, // ����
	//	{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f } }, // ����
	//	{ {  1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f } }, // �E��
	//	{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f } }, // �E��
	//};

	//D3D12_HEAP_PROPERTIES heapProp{};
	//heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	//heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	//heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	//D3D12_RESOURCE_DESC resDesc{};
	//resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	//resDesc.Width = sizeof(vertices); // ���_��񂪓��邾���̃T�C�Y
	//resDesc.Height = 1;
	//resDesc.DepthOrArraySize = 1;
	//resDesc.MipLevels = 1;
	//resDesc.Format = DXGI_FORMAT_UNKNOWN;
	//resDesc.SampleDesc.Count = 1;
	//resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	//resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ComPtr<ID3D12Resource> vertBuff{ nullptr };

	//result = _dev->CreateCommittedResource(
	//	&heapProp,
	//	D3D12_HEAP_FLAG_NONE,
	//	&resDesc,
	//	D3D12_RESOURCE_STATE_GENERIC_READ,
	//	nullptr,
	//	IID_PPV_ARGS(vertBuff.ReleaseAndGetAddressOf()));

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

	// �C���f�b�N�X�o�b�t�@�쐬
	//unsigned short indices[] =
	//{
	//	0, 1, 2,
	//	2, 1, 3
	//};

	ComPtr<ID3D12Resource> idxBuff{ nullptr };

	//// �ݒ�́A�o�b�t�@�̃T�C�Y�ȊO�A���_�o�b�t�@�̐ݒ���g���܂킵�Ă悢
	//resDesc.Width = sizeof(indices);

	//result = _dev->CreateCommittedResource(
	//	&heapProp,
	//	D3D12_HEAP_FLAG_NONE,
	//	&resDesc,
	//	D3D12_RESOURCE_STATE_GENERIC_READ,
	//	nullptr,
	//	IID_PPV_ARGS(idxBuff.ReleaseAndGetAddressOf()));

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
		{ "BONE_NO",  0, DXGI_FORMAT_R16G16_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // �{�[���ԍ�
		{ "WEIGHT",   0, DXGI_FORMAT_R8_UINT,         0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // �E�F�C�g
	};

	//// �e�N�X�`���f�[�^�̍쐬
	//struct TexRGBA
	//{
	//	unsigned char R, G, B, A;
	//};

	//std::vector<TexRGBA> textureData(256 * 256);
	//for (auto& rgba : textureData)
	//{
	//	rgba.R = rand() % 256;
	//	rgba.G = rand() % 256;
	//	rgba.B = rand() % 256;
	//	rgba.A = 255; // ����1.0f�Ƃ���
	//}

	//// WIC�e�N�X�`���̃��[�h
	//TexMetadata metaData{};
	//ScratchImage scratchImg{};

	//result = LoadFromWICFile(
	//	L"Image/textest.png", WIC_FLAGS_NONE,
	//	&metaData, scratchImg);

	//auto img = scratchImg.GetImage(0, 0, 0);

	//// �����Ő�������Ă�����@
	//// ���ԃo�b�t�@�Ƃ��ẴA�b�v���[�h�q�[�v�ݒ�
	//D3D12_HEAP_PROPERTIES uploadHeapProp{};
	//uploadHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD; // �}�b�v�\�ɂ��邽�߁AUPLOAD�ɂ���
	//uploadHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN; // �A�b�v���[�h�p�Ɏg�p���邱�ƑO��Ȃ̂�UNKNOWN�ł悢
	//uploadHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	//uploadHeapProp.CreationNodeMask = 0; // �P��A�_�v�^�̂��� 0
	//uploadHeapProp.VisibleNodeMask = 0;  // �P��A�_�v�^�̂��� 0

	//ComPtr<ID3D12Resource> uploadBuff{ nullptr };

	{
		//D3D12_RESOURCE_DESC resDesc{};
		//resDesc.Format = DXGI_FORMAT_UNKNOWN; // �P�Ȃ�f�[�^�̉�Ȃ̂� UNKNOWN
		//resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; // �P�Ȃ�o�b�t�@�[�Ƃ��Ďw��
		//resDesc.Width = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) * img->height; // �f�[�^�T�C�Y
		//resDesc.Height = 1;
		//resDesc.DepthOrArraySize = 1;
		//resDesc.MipLevels = 1;
		//resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // �A�������f�[�^
		//resDesc.Flags = D3D12_RESOURCE_FLAG_NONE; // ���Ɏw��Ȃ�
		//resDesc.SampleDesc.Count = 1; // �ʏ�e�N�X�`���Ȃ̂ŃA���`�G�C���A�V���O���Ȃ�
		//resDesc.SampleDesc.Quality = 0;
		//result = _dev->CreateCommittedResource(
		//	&uploadHeapProp,
		//	D3D12_HEAP_FLAG_NONE, // ���Ɏw��Ȃ�
		//	&resDesc,
		//	D3D12_RESOURCE_STATE_GENERIC_READ,
		//	nullptr,
		//	IID_PPV_ARGS(uploadBuff.ReleaseAndGetAddressOf()));
	}

	//ComPtr<ID3D12Resource> texBuff{ nullptr };

	// ���heapProp���g���Ă�̂Ŗ������B�i�ȒP�ȕ��j
	{
		//// WriteToSubresource�œ]�����邽�߂̃q�[�v�ݒ�
		//D3D12_HEAP_PROPERTIES heapProp{};

		//// ����Ȑݒ�Ȃ̂�DEFAULT�ł�UPLOAD�ł��Ȃ�
		//heapProp.Type = D3D12_HEAP_TYPE_CUSTOM;

		//// ���C�g�o�b�N
		//heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;

		//// �]����L0�A�܂�CPU�����璼�ڍs��
		//heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

		//// �P��A�_�v�^�[�̂��� 0
		//heapProp.CreationNodeMask = 0;
		//heapProp.VisibleNodeMask = 0;

		//D3D12_RESOURCE_DESC resDesc{};
		//resDesc.Format = metaData.format; // RGBA�t�H�[�}�b�g
		//resDesc.Width = metaData.width; // ��
		//resDesc.Height = metaData.height; // ����
		//resDesc.DepthOrArraySize = metaData.arraySize; // 2D�Ŕz��ł��Ȃ��̂� 1
		//resDesc.SampleDesc.Count = 1; // �ʏ�e�N�X�`���Ȃ̂ŃA���`�G�C���A�V���O���Ȃ�
		//resDesc.SampleDesc.Quality = 0; // �N�I���e�B�͍Œ�
		//resDesc.MipLevels = metaData.mipLevels;          // �~�b�v�}�b�v���Ȃ��̂Ń~�b�v����1��
		//resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metaData.dimension); // 2D�e�N�X�`��
		//resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // ���C�A�E�g�͌��肵�Ȃ�
		//resDesc.Flags = D3D12_RESOURCE_FLAG_NONE; // ���Ɏw��Ȃ�

		//result = _dev->CreateCommittedResource(
		//	&heapProp,
		//	D3D12_HEAP_FLAG_NONE, // ���Ɏw��Ȃ�
		//	&resDesc,
		//	D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, // �V�F�[�_�[���\�[�X�Ƃ��č쐬
		//	nullptr,
		//	IID_PPV_ARGS(texBuff.ReleaseAndGetAddressOf()));

		//result = texBuff->WriteToSubresource(
		//	0,
		//	nullptr, // �S�̈�R�s�[
		//	img->pixels, // ���f�[�^�A�h���X
		//	img->rowPitch, // 1���C���T�C�Y
		//	img->slicePitch); // �S�T�C�Y
	}

	// �����Ő�������Ă�����i����j
	{
		//// ������̂����p�̃q�[�v�ݒ�
		//D3D12_HEAP_PROPERTIES texHeapProp{};
		//texHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT; // �e�N�X�`���p
		//texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		//texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		//texHeapProp.CreationNodeMask = 0;
		//texHeapProp.VisibleNodeMask = 0;

		//D3D12_RESOURCE_DESC resDesc{};
		//resDesc.Format = metaData.format; // RGBA�t�H�[�}�b�g
		//resDesc.Width = metaData.width; // ��
		//resDesc.Height = metaData.height; // ����
		//resDesc.DepthOrArraySize = metaData.arraySize; // 2D�Ŕz��ł��Ȃ��̂� 1
		//resDesc.SampleDesc.Count = 1; // �ʏ�e�N�X�`���Ȃ̂ŃA���`�G�C���A�V���O���Ȃ�
		//resDesc.SampleDesc.Quality = 0; // �N�I���e�B�͍Œ�
		//resDesc.MipLevels = metaData.mipLevels;          // �~�b�v�}�b�v���Ȃ��̂Ń~�b�v����1��
		//resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metaData.dimension); // 2D�e�N�X�`��
		//resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // ���C�A�E�g�͌��肵�Ȃ�
		//resDesc.Flags = D3D12_RESOURCE_FLAG_NONE; // ���Ɏw��Ȃ�

		//result = _dev->CreateCommittedResource(
		//	&texHeapProp,
		//	D3D12_HEAP_FLAG_NONE, // ���Ɏw��Ȃ�
		//	&resDesc,
		//	D3D12_RESOURCE_STATE_COPY_DEST, // �R�s�[��
		//	nullptr,
		//	IID_PPV_ARGS(texBuff.ReleaseAndGetAddressOf()));

		//decltype(img->pixels) mapforImg = nullptr;
		//result = uploadBuff->Map(0, nullptr, (void**)&mapforImg);
		////std::copy_n(img->pixels, img->slicePitch, mapforImg);

		//auto srcAddress = img->pixels;
		//auto rowPitch = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

		//for (int y = 0; y < img->height; ++y)
		//{
		//	std::copy_n(srcAddress, rowPitch, mapforImg); // �R�s�[

		//	// 1�s���Ƃɂ��܂����킹��
		//	srcAddress += img->rowPitch;
		//	mapforImg += rowPitch;
		//}

		//uploadBuff->Unmap(0, nullptr);

		//D3D12_TEXTURE_COPY_LOCATION src{};
		//// �R�s�[���i�A�b�v���[�h���j�ݒ�
		//src.pResource = uploadBuff.Get(); // ���ԃo�b�t�@
		//src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT; // �t�b�g�v�����g�w��
		//src.PlacedFootprint.Offset = 0;
		//src.PlacedFootprint.Footprint.Width = metaData.width;
		//src.PlacedFootprint.Footprint.Height = metaData.height;
		//src.PlacedFootprint.Footprint.Depth = metaData.depth;
		//src.PlacedFootprint.Footprint.RowPitch = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
		//src.PlacedFootprint.Footprint.Format = img->format;

		//D3D12_TEXTURE_COPY_LOCATION dst{};
		//// �R�s�[��ݒ�
		//dst.pResource = texBuff.Get();
		//dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		//dst.SubresourceIndex = 0;

		//_cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

		//D3D12_RESOURCE_BARRIER barrierDesc{};
		//barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		//barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		//barrierDesc.Transition.pResource = texBuff.Get();
		//barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		//barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST; // �d�v
		//barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; // �d�v

		//_cmdList->ResourceBarrier(1, &barrierDesc);
		//_cmdList->Close();

		//// �R�}���h���X�g�̎��s
		//ID3D12CommandList* cmdLists[]{ _cmdList.Get() };
		//_cmdQueue->ExecuteCommandLists(1, cmdLists);

		//// ��������
		//_cmdQueue->Signal(_fence.Get(), ++_fenceVal);
		//if (_fence->GetCompletedValue() != _fenceVal)
		//{
		//	// �C�x���g�n���h���̎擾
		//	auto event = CreateEvent(nullptr, false, false, nullptr);

		//	_fence->SetEventOnCompletion(_fenceVal, event);

		//	// �C�x���g����������܂ő҂�������iINFINITE�j
		//	WaitForSingleObject(event, INFINITE);

		//	// �C�x���g�n���h�������
		//	CloseHandle(event);
		//}
		//_cmdAllocator->Reset(); // �L���[�̃N���A
		//_cmdList->Reset(_cmdAllocator.Get(), nullptr); // �ĂуR�}���h���X�g�����߂鏀��
	}

	ComPtr<ID3D12DescriptorHeap> basicDescHeap{ nullptr };
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // �V�F�[�_�[���猩����悤��
	descHeapDesc.NodeMask = 0; // �}�X�N��0
	descHeapDesc.NumDescriptors = 2; // SRV, CBV
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; // �V�F�[�_�[���\�[�X�r���[�p
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(basicDescHeap.ReleaseAndGetAddressOf()));

	//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	//srvDesc.Format = metaData.format; // RGBA�i0.0f�`1.0f�ɐ��K���j
	//srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2D�e�N�X�`��
	//srvDesc.Texture2D.MipLevels = 1; // �~�b�v�}�b�v�͎g�p���Ȃ��̂� 1

	auto basicHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart();

	//_dev->CreateShaderResourceView(
	//	texBuff.Get(), // �r���[�Ɗ֘A�t����o�b�t�@
	//	&srvDesc, // ��قǐݒ肵���e�N�X�`���ݒ���
	//	basicHeapHandle); // �q�[�v�̂ǂ��Ɋ��蓖�Ă邩

	//basicHeapHandle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = constBuff->GetDesc().Width;

	// �萔�o�b�t�@�r���[�쐬
	_dev->CreateConstantBufferView(
		&cbvDesc,
		basicHeapHandle);

	// �����W�̍쐬
	D3D12_DESCRIPTOR_RANGE range{};
	
	//// �e�N�X�`���p
	//range[0].NumDescriptors = 1; // �e�N�X�`��1��
	//range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // ��ʂ̓e�N�X�`��
	//range[0].BaseShaderRegister = 0; // 0�ԃX���b�g����
	//range[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

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
		*mapMat = worldMat * viewMat * projMat;

		// ���݂̃o�b�N�o�b�t�@�[�̃C���f�b�N�X���擾
		auto bbIdx = _swapChain->GetCurrentBackBufferIndex();

		{
			//D3D12_RESOURCE_BARRIER barrierDesc{};
			//barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION; // �J��
			//barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE; // ���Ɏw��Ȃ�
			//barrierDesc.Transition.pResource = _backBuffers[bbIdx].Get();
			//barrierDesc.Transition.Subresource = 0;
			//barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT; // ���O�� PRESENT ���
			//barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET; // �����烌���_�[�^�[�Q�b�g���
			//_cmdList->ResourceBarrier(1, &barrierDesc);
		}

		_cmdList->ResourceBarrier(
			1, &CD3DX12_RESOURCE_BARRIER::Transition(
				_backBuffers[bbIdx].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		// �����_�[�^�[�Q�b�g�r���[���Z�b�g
		auto rtvH = _rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		_cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

		// �����_�[�^�[�Q�b�g�̃N���A
		float clearColor[4]{ 1.0f, 1.0f, 1.0f, 1.0f }; // �w�i�F�i���͉��F�j
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

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

		// ���[�g�V�O�l�`�����Z�b�g
		_cmdList->SetGraphicsRootSignature(rootSignature.Get());

		// �f�B�X�N���v�^�q�[�v���Z�b�g
		_cmdList->SetDescriptorHeaps(1, basicDescHeap.GetAddressOf());

		// ���[�g�p�����[�^�[�ƃf�B�X�N���v�^�q�[�v�̊֘A�t��
		_cmdList->SetGraphicsRootDescriptorTable(
			0, // ���[�g�p�����[�^�[�C���f�b�N�X
			basicDescHeap->GetGPUDescriptorHandleForHeapStart()); // �q�[�v�A�h���X

		// �h���[�R�[��
		_cmdList->DrawIndexedInstanced(indicesNum, 1, 0, 0, 0);

		{
			//barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			//barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
			//_cmdList->ResourceBarrier(1, &barrierDesc);
		}

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

