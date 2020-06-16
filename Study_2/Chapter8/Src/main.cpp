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

#include <algorithm>
#include <map>

constexpr size_t pmdvertex_size = 38; // 1���_������̃T�C�Y

using LoadLambda_t = std::function<HRESULT(const std::wstring& path, TexMetadata*, ScratchImage&)>;

std::map<std::string, LoadLambda_t> loadLambdaTable;
std::map<std::string, ComPtr<ID3D12Resource>> _resourceTable;

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

// ���f���̃p�X�ƃe�N�X�`���̃p�X���獇���p�X�𓾂�
// @param modelPath �A�v���P�[�V�������猩��pmd���f���p�X
// @texPath PMD���f�����猩���e�N�X�`���̃p�X
// @return �A�v���P�[�V�������猩���e�N�X�`���̃p�X
std::string GetTexturePathFromModelAndTexPath(
	const std::string& modelPath, const char* texPath)
{
	// �t�@�C���̃t�H���_�[��؂�́u\�v�Ɓu/�v��2���
	int pathIndex1 = modelPath.rfind('/');
	int pathIndex2 = modelPath.rfind('\\');

	auto pathIndex = max(pathIndex1, pathIndex2);
	auto folderPath = modelPath.substr(0, pathIndex + 1);

	return folderPath + texPath;
}

// std::string�i�}���`�o�C�g������j����std::wstring�i���C�h������j�𓾂�
// @param str �}���`�o�C�g������
// @return �ϊ����ꂽ���C�h������
std::wstring GetWideStringFromString(const std::string& str)
{
	// �Ăяo��1��ځi�����񐔂𓾂�j
	auto num1 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		nullptr,
		0);

	std::wstring wstr; // string��wchar_t��
	wstr.resize(num1);

	// �Ăяo��2��ځi�m�ۍς݂�wstr�ɕϊ���������R�s�[�j
	auto num2 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		&wstr[0],
		num1);

	assert(num1 == num2); // �ꉞ�`�F�b�N
	return wstr;
}

// �t�@�C��������g���q���擾����
// @param path �Ώۂ̃p�X������
// @return �g���q
std::string GetExtension(const std::string& path)
{
	int idx = path.rfind('.');
	return path.substr(idx + 1, path.length() - idx - 1);
}

// �e�N�X�`���̃p�X���Z�p���[�^�[�����ŕ�������
// @param path �Ώۂ̃p�X������
// @param splitter ��؂蕶��
// @return �����O��̕�����y�A
std::pair<std::string, std::string> SplitFileName(
	const std::string& path, const char splitter = '*')
{
	int idx = path.find(splitter);
	pair<std::string, std::string> ret;
	ret.first = path.substr(0, idx);
	ret.second = path.substr(idx + 1, path.length() - idx - 1);
	return ret;
}

ComPtr<ID3D12Resource> LoadTextureFromFile(std::string& texPath, ComPtr<ID3D12Device> dev)
{
	auto it = _resourceTable.find(texPath);

	// �e�[�u�����ɂ������烍�[�h����̂ł͂Ȃ�
	// �}�b�v���̃��\�[�X��Ԃ�
	if (it != _resourceTable.end())
	{
		return it->second;
	}

	// WIC�e�N�X�`���̃��[�h
	TexMetadata metadata{};
	ScratchImage scratchImg{};

	auto wtexPath = GetWideStringFromString(texPath); // �e�N�X�`���̃t�@�C���p�X
	auto ext = GetExtension(texPath);

	if (ext == "")
	{
		return nullptr;
	}

	auto result = loadLambdaTable[ext](
		wtexPath,
		&metadata,
		scratchImg);

	if (FAILED(result))
	{
		return nullptr;
	}

	auto img = scratchImg.GetImage(0, 0, 0); // ���f�[�^���o

	// WriteToSubresource�œ]������p�̃q�[�v�ݒ�
	D3D12_HEAP_PROPERTIES texHeapProp{};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	texHeapProp.CreationNodeMask = 0; // �P��A�_�v�^�[�̂���0
	texHeapProp.VisibleNodeMask = 0; // ��ɓ���
	
	D3D12_RESOURCE_DESC resDesc{};
	resDesc.Format = metadata.format;
	resDesc.Width = metadata.width;
	resDesc.Height = metadata.height;
	resDesc.DepthOrArraySize = metadata.arraySize;
	resDesc.SampleDesc.Count = 1; // �ʏ�e�N�X�`���Ȃ̂ŃA���`�G�C���A�V���O���Ȃ�
	resDesc.SampleDesc.Quality = 0; // �N�I���e�B�͍Œ�
	resDesc.MipLevels = metadata.mipLevels;
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // ���C�A�E�g�͌��肵�Ȃ�
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE; // ���Ƀt���O�Ȃ�

	// �o�b�t�@�쐬
	ComPtr<ID3D12Resource> texBuff{ nullptr };
	result = dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE, // ���Ɏw��Ȃ�
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(texBuff.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		return nullptr;
	}

	result = texBuff->WriteToSubresource(
		0,
		nullptr,
		img->pixels,
		img->rowPitch,
		img->slicePitch);

	if (FAILED(result))
	{
		return nullptr;
	}

	_resourceTable[texPath] = texBuff;

	return texBuff;
}

// ������3�͗v�`�F�b�N����
ComPtr<ID3D12Resource> CreateWhiteTexture(ComPtr<ID3D12Device> dev)
{
	// WriteToSubresource�œ]������p�̃q�[�v�ݒ�
	D3D12_HEAP_PROPERTIES texHeapProp{};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	//texHeapProp.CreationNodeMask = 0; // �P��A�_�v�^�[�̂���0
	texHeapProp.VisibleNodeMask = 0; // ��ɓ���

	D3D12_RESOURCE_DESC resDesc{};
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Width = 4;
	resDesc.Height = 4;
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1; // �ʏ�e�N�X�`���Ȃ̂ŃA���`�G�C���A�V���O���Ȃ�
	resDesc.SampleDesc.Quality = 0; // �N�I���e�B�͍Œ�
	resDesc.MipLevels = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // ���C�A�E�g�͌��肵�Ȃ�
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE; // ���Ƀt���O�Ȃ�

	// �o�b�t�@�쐬
	ComPtr<ID3D12Resource> texBuff{ nullptr };
	auto result = dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE, // ���Ɏw��Ȃ�
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(texBuff.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		return nullptr;
	}

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0xff); // �S��255�Ŗ��߂�i���j

	result = texBuff->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * 4,
		data.size());

	if (FAILED(result))
	{
		return nullptr;
	}

	return texBuff;
}

ComPtr<ID3D12Resource> CreateBlackTexture(ComPtr<ID3D12Device> dev)
{
	// WriteToSubresource�œ]������p�̃q�[�v�ݒ�
	D3D12_HEAP_PROPERTIES texHeapProp{};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	//texHeapProp.CreationNodeMask = 0; // �P��A�_�v�^�[�̂���0
	texHeapProp.VisibleNodeMask = 0; // ��ɓ���

	D3D12_RESOURCE_DESC resDesc{};
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Width = 4;
	resDesc.Height = 4;
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1; // �ʏ�e�N�X�`���Ȃ̂ŃA���`�G�C���A�V���O���Ȃ�
	resDesc.SampleDesc.Quality = 0; // �N�I���e�B�͍Œ�
	resDesc.MipLevels = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // ���C�A�E�g�͌��肵�Ȃ�
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE; // ���Ƀt���O�Ȃ�

	// �o�b�t�@�쐬
	ComPtr<ID3D12Resource> texBuff{ nullptr };
	auto result = dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE, // ���Ɏw��Ȃ�
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(texBuff.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		return nullptr;
	}

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0x00); // �S��0�Ŗ��߂�i���j

	result = texBuff->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * 4,
		data.size());

	if (FAILED(result))
	{
		return nullptr;
	}

	return texBuff;
}

ComPtr<ID3D12Resource> CreateGrayGradationTexture(ComPtr<ID3D12Device> dev)
{
	// WriteToSubresource�œ]������p�̃q�[�v�ݒ�
	D3D12_HEAP_PROPERTIES texHeapProp{};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	//texHeapProp.CreationNodeMask = 0; // �P��A�_�v�^�[�̂���0
	texHeapProp.VisibleNodeMask = 0; // ��ɓ���

	D3D12_RESOURCE_DESC resDesc{};
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Width = 4;
	resDesc.Height = 256;
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1; // �ʏ�e�N�X�`���Ȃ̂ŃA���`�G�C���A�V���O���Ȃ�
	resDesc.SampleDesc.Quality = 0; // �N�I���e�B�͍Œ�
	resDesc.MipLevels = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // ���C�A�E�g�͌��肵�Ȃ�
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE; // ���Ƀt���O�Ȃ�

	// �o�b�t�@�쐬
	ComPtr<ID3D12Resource> texBuff{ nullptr };
	auto result = dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE, // ���Ɏw��Ȃ�
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(texBuff.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		return nullptr;
	}

	// �オ�����ĉ��������e�N�X�`���f�[�^���쐬
	std::vector<unsigned int> data(4 * 256);
	auto it = data.begin();
	unsigned int c = 0xff;
	for (; it != data.end(); it += 4)
	{
		auto col = (c << 0xff) | (c << 16) | (c << 8) | c;
		std::fill(it, it + 4, col);
		--c;
	}

	result = texBuff->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * sizeof(unsigned int),
		sizeof(unsigned int) * data.size());

	if (FAILED(result))
	{
		return nullptr;
	}

	return texBuff;
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

	loadLambdaTable["sph"]
		= loadLambdaTable["spa"]
		= loadLambdaTable["bmp"]
		= loadLambdaTable["png"]
		= loadLambdaTable["jpg"]
		= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)
		->HRESULT
	{
		return LoadFromWICFile(path.c_str(), 0, meta, img);
	};

	loadLambdaTable["tga"]
		= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)
		->HRESULT
	{
		return LoadFromTGAFile(path.c_str(), meta, img);
	};

	loadLambdaTable["dds"]
		= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)
		->HRESULT
	{
		return LoadFromDDSFile(path.c_str(), 0, meta, img);
	};

	// ==========DirectX12�֌W==========

#ifdef _DEBUG

	// �f�o�b�O���C���[���I����
	EnableDebugLayer();

#endif

	ComPtr<ID3D12Device>    _dev{ nullptr };
	ComPtr<IDXGIFactory6>   _dxgiFactory{ nullptr };
	ComPtr<IDXGISwapChain4> _swapChain{ nullptr };

	// �t�@�N�g���[�̍쐬
#ifdef _DEBUG

	// DXGI�֌W�̃G���[���b�Z�[�W��\���ł���
	auto result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf()));

#else

	auto result = CreateDXGIFactory1(IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf()));

#endif

	std::vector<ComPtr<IDXGIAdapter>> adapters;
	ComPtr<IDXGIAdapter> tmpAdapter{ nullptr };

	for (int i = 0; _dxgiFactory->EnumAdapters(i, tmpAdapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(tmpAdapter);
	}

	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc{};
		adpt->GetDesc(&adesc);
		std::wstring strDesc = adesc.Description;

		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}

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
		// ��1������nullptr�ɂ���ƃ_���B
		// Intel(R) UHD Graphics 630�ł����܂������Ȃ��B���f����������
		// ����� "NVIDIA" �����A�_�v�^�[���w��B
		if (D3D12CreateDevice(tmpAdapter.Get(), lv, IID_PPV_ARGS(_dev.ReleaseAndGetAddressOf())) == S_OK)
		{
			featureLevel = lv;
			break; // �����\�ȃo�[�W���������������烋�[�v��ł��؂�
		}
	}

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
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // �K���}�␳����isRGB�j
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
	depthResDesc.Height = window_height; // 
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

#pragma pack(1)

	// PMD�}�e���A���\����
	struct PMDMaterial
	{
		XMFLOAT3 diffuse;        // �f�B�t���[�Y�F
		float alpha;             // �f�B�t���[�Y��
		float specularity;       // �X�y�L�����̋����i��Z�l�j
		XMFLOAT3 specular;       // �X�y�L�����F
		XMFLOAT3 ambient;        // �A���r�G���g�F
		unsigned char toonIdx;   // �g�D�[���ԍ�
		unsigned char edgeFlg;   // �}�e���A�����Ƃ̗֊s���t���O
		unsigned int indicesNum; // ���̃}�e���A�������蓖�Ă���C���f�b�N�X��
		char texFilePath[20];    // �e�N�X�`���t�@�C���p�X+��
	};

#pragma pack()

	// �V�F�[�_�[���ɓ�������}�e���A���f�[�^
	struct MaterialForHlsl
	{
		XMFLOAT3 diffuse;  // �f�B�t���[�Y�F
		float alpha;	   // �f�B�t���[�Y��
		XMFLOAT3 specular; // �X�y�L�����F
		float specularity; // �X�y�L�����̋����i��Z�l�j
		XMFLOAT3 ambient;  // �A���r�G���g�F
	};

	// ����ȊO�̃}�e���A���f�[�^
	struct AdditionalMaterial
	{
		std::string texPath; // �e�N�X�`���t�@�C���p�X
		int toonIdx;         // �g�D�[���ԍ�
		bool edgeFlg;        // �}�e���A�����Ƃ̗֊s���t���O
	};

	// �S�̂��܂Ƃ߂�f�[�^
	struct Material
	{
		unsigned int indicesNum; // �C���f�b�N�X��
		MaterialForHlsl material;
		AdditionalMaterial additional;
	};

	PMDHeader pmdHeader;
	char signature[3]{}; // pmd�t�@�C���̐擪3�o�C�g���uPmd�v�ƂȂ��Ă��邽�߁A����͍\���̂Ɋ܂߂Ȃ�
	FILE* fp;
	unsigned int vertNum; // ���_��
	std::vector<unsigned char> vertices; // �o�b�t�@�̊m��
	unsigned int indicesNum; // �C���f�b�N�X��
	std::vector<unsigned short> indices;
	unsigned int materialNum; // �}�e���A����
	std::vector<PMDMaterial> pmdMaterials;
	std::vector<Material> materials;

	std::string strModelPath = "Model/�����~�N.pmd";
	fopen_s(&fp, strModelPath.c_str(), "rb");
	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdHeader, sizeof(pmdHeader), 1, fp);

	fread(&vertNum, sizeof(vertNum), 1, fp);
	vertices.resize(vertNum * pmdvertex_size);
	fread(vertices.data(), vertices.size(), 1, fp);

	fread(&indicesNum, sizeof(indicesNum), 1, fp);
	indices.resize(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	fread(&materialNum, sizeof(materialNum), 1, fp);
	pmdMaterials.resize(materialNum);
	fread(pmdMaterials.data(), pmdMaterials.size() * sizeof(PMDMaterial), 1, fp);

	fclose(fp);

	materials.resize(pmdMaterials.size());

	// �R�s�[
	for (int i = 0; i < pmdMaterials.size(); ++i)
	{
		materials[i].indicesNum           = pmdMaterials[i].indicesNum;
		materials[i].material.diffuse     = pmdMaterials[i].diffuse;
		materials[i].material.alpha       = pmdMaterials[i].alpha;
		materials[i].material.specular    = pmdMaterials[i].specular;
		materials[i].material.specularity = pmdMaterials[i].specularity;
		materials[i].material.ambient     = pmdMaterials[i].ambient;
	}

	ComPtr<ID3D12Resource> whiteTex = CreateWhiteTexture(_dev);
	ComPtr<ID3D12Resource> blackTex = CreateBlackTexture(_dev);
	ComPtr<ID3D12Resource> gradTex = CreateGrayGradationTexture(_dev);

	// PMD���f���̃e�N�X�`�������[�h
	std::vector<ComPtr<ID3D12Resource>> texResources;
	std::vector<ComPtr<ID3D12Resource>> sphResources;
	std::vector<ComPtr<ID3D12Resource>> spaResources;
	std::vector<ComPtr<ID3D12Resource>> toonResources;

	texResources.resize(materialNum);
	sphResources.resize(materialNum);
	spaResources.resize(materialNum);
	toonResources.resize(materialNum);

	for (int i = 0; i < pmdMaterials.size(); ++i)
	{
		// �g�D�[�����\�[�X�̓ǂݍ���
		std::string toonFilePath = "Toon/";
		char toonFileName[16];
		sprintf_s(
			toonFileName,
			"toon%02d.bmp",
			pmdMaterials[i].toonIdx + 1);

		toonFilePath += toonFileName;
		toonResources[i] = LoadTextureFromFile(toonFilePath, _dev);

		if (strlen(pmdMaterials[i].texFilePath) == 0)
		{
			continue;
		}

		std::string fileName = pmdMaterials[i].texFilePath;
		std::string texFileName = "";
		std::string sphFileName = "";
		std::string spaFileName = "";

		if (std::count(fileName.begin(), fileName.end(), '*') > 0)
		{ // �X�v���b�^������
			auto namePair = SplitFileName(fileName);

			if (GetExtension(namePair.first) == "sph")
			{
				texFileName = namePair.second;
				sphFileName = namePair.first;
			}
			else if (GetExtension(namePair.first) == "spa")
			{
				texFileName = namePair.second;
				spaFileName = namePair.first;
			}
			else
			{
				texFileName = namePair.first;

				if (GetExtension(namePair.second) == "sph")
				{
					sphFileName = namePair.second;
				}
				else if (GetExtension(namePair.second) == "spa")
				{
					spaFileName = namePair.second;
				}
			}
		}
		else
		{
			if (GetExtension(pmdMaterials[i].texFilePath) == "sph")
			{
				sphFileName = pmdMaterials[i].texFilePath;
			}
			else if (GetExtension(pmdMaterials[i].texFilePath) == "spa")
			{
				spaFileName = pmdMaterials[i].texFilePath;;
			}
			else
			{
				texFileName = pmdMaterials[i].texFilePath;
			}
		}

		// ���f���ƃe�N�X�`���p�X����A�v���P�[�V��������̃e�N�X�`���p�X�𓾂�
		if (texFileName != "")
		{
			auto texFilePath = GetTexturePathFromModelAndTexPath(
				strModelPath,
				texFileName.c_str());

			texResources[i] = LoadTextureFromFile(texFilePath, _dev);
		}
		if (sphFileName != "")
		{
			auto sphFilePath = GetTexturePathFromModelAndTexPath(
				strModelPath,
				sphFileName.c_str());

			sphResources[i] = LoadTextureFromFile(sphFilePath, _dev);
		}
		if (spaFileName != "")
		{
			auto spaFilePath = GetTexturePathFromModelAndTexPath(
				strModelPath,
				spaFileName.c_str());

			spaResources[i] = LoadTextureFromFile(spaFilePath, _dev);
		}
	}

	// �}�e���A���o�b�t�@�[�쐬
	auto materialBuffSize = (sizeof(MaterialForHlsl) + 0xff) & ~0xff;

	ComPtr<ID3D12Resource> materialBuff{ nullptr };
	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(materialBuffSize * materialNum), // ���������Ȃ�����
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(materialBuff.ReleaseAndGetAddressOf()));

	// �}�b�v�}�e���A���ɃR�s�[
	char* mapMaterial{ nullptr };
	result = materialBuff->Map(0, nullptr, (void**)&mapMaterial);

	for (auto& m : materials)
	{
		*((MaterialForHlsl*)mapMaterial) = m.material; // �f�[�^�R�s�[
		mapMaterial += materialBuffSize; // ���̃A���C�������g�ʒu�܂Ői�߂�i256�̔{���j
	}

	materialBuff->Unmap(0, nullptr);

	// �}�e���A���p�̃f�B�X�N���v�^�q�[�v�쐬
	ComPtr<ID3D12DescriptorHeap> materialDescHeap{ nullptr };

	D3D12_DESCRIPTOR_HEAP_DESC matDescHeapDesc{};
	matDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matDescHeapDesc.NodeMask = 0;
	matDescHeapDesc.NumDescriptors = materialNum * 5; // �}�e���A���� * 5���w��
	matDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	result = _dev->CreateDescriptorHeap(&matDescHeapDesc, IID_PPV_ARGS(materialDescHeap.ReleaseAndGetAddressOf()));

	// �}�e���A���v�萔�o�b�t�@�[�r���[�쐬
	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc{};
	matCBVDesc.BufferLocation = materialBuff->GetGPUVirtualAddress(); // �o�b�t�@�A�h���X
	matCBVDesc.SizeInBytes = materialBuffSize; // �}�e���A����256�A���C�������g�T�C�Y

	// �}�e���A���v�V�F�[�_�[���\�[�X�r���[�쐬
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // �f�t�H���g
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2D�e�N�X�`��
	srvDesc.Texture2D.MipLevels = 1; // �~�b�v�}�b�v�͎g�p���Ȃ��̂� 1

	auto matDescHeapH = materialDescHeap->GetCPUDescriptorHandleForHeapStart(); // �擪���L�^
	auto inc = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (int i = 0; i < materialNum; ++i)
	{
		// �萔�o�b�t�@�[�r���[
		_dev->CreateConstantBufferView(&matCBVDesc, matDescHeapH);
		matDescHeapH.ptr += inc;
		matCBVDesc.BufferLocation += materialBuffSize;

		// �V�F�[�_�[���\�[�X�r���[
		if (texResources[i] != nullptr)
		{
			srvDesc.Format = texResources[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(
				texResources[i].Get(),
				&srvDesc,
				matDescHeapH);
		}
		else
		{
			srvDesc.Format = whiteTex->GetDesc().Format;
			_dev->CreateShaderResourceView(
				whiteTex.Get(),
				&srvDesc,
				matDescHeapH);
		}

		matDescHeapH.ptr += inc;

		if (sphResources[i] != nullptr)
		{
			srvDesc.Format = sphResources[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(
				sphResources[i].Get(),
				&srvDesc,
				matDescHeapH);
		}
		else
		{
			srvDesc.Format = whiteTex->GetDesc().Format;
			_dev->CreateShaderResourceView(
				whiteTex.Get(),
				&srvDesc,
				matDescHeapH);
		}

		matDescHeapH.ptr += inc;

		if (spaResources[i] != nullptr)
		{
			srvDesc.Format = spaResources[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(
				spaResources[i].Get(),
				&srvDesc,
				matDescHeapH);
		}
		else
		{
			srvDesc.Format = blackTex->GetDesc().Format;
			_dev->CreateShaderResourceView(
				blackTex.Get(),
				&srvDesc,
				matDescHeapH);
		}

		matDescHeapH.ptr += inc;

		if (toonResources[i] != nullptr)
		{
			srvDesc.Format = toonResources[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(
				toonResources[i].Get(),
				&srvDesc,
				matDescHeapH);
		}
		else
		{
			srvDesc.Format = gradTex->GetDesc().Format;
			_dev->CreateShaderResourceView(
				gradTex.Get(),
				&srvDesc,
				matDescHeapH);
		}

		matDescHeapH.ptr += inc;
	}

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
	std::copy(vertices.begin(), vertices.end(), vertMap);
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
	std::copy(indices.begin(), indices.end(), mappedIdx);
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
	descHeapDesc.NumDescriptors = 1; // CBV
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(basicDescHeap.ReleaseAndGetAddressOf()));

	// �V�F�[�_�[���ɓn�����߂̊�{�I�Ȋ��f�[�^
	struct SceneMatrix
	{
		XMMATRIX world; // ���f���{�̂���]��������ړ��������肷��s��
		XMMATRIX view;  // �r���[�s��
		XMMATRIX proj;  // �v���W�F�N�V�����s��
		XMFLOAT3 eye;   // ���_���W
	};

	XMMATRIX worldMat = XMMatrixIdentity();
	XMFLOAT3 eye{ 0.0f, 17.0f, -10.0f };
	XMFLOAT3 target{ 0.0f, 18.0f, 0.0f };
	XMFLOAT3 up{ 0.0f, 1.0f, 0.0f };

	worldMat = XMMatrixRotationY(XM_PIDIV4);
	auto viewMat = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
	auto projMat = XMMatrixPerspectiveFovLH(
		XM_PIDIV4,
		static_cast<float>(window_width) / static_cast<float>(window_height),
		1.0f,
		100.0f);

	// �萔�o�b�t�@�쐬
	ComPtr<ID3D12Resource> constBuff{ nullptr };

	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneMatrix) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(constBuff.ReleaseAndGetAddressOf()));

	SceneMatrix* mapMat{ nullptr }; // �}�b�v��������|�C���^�[
	result = constBuff->Map(0, nullptr, (void**)&mapMat); // �}�b�v
	mapMat->world = worldMat; // �s��̓��e���R�s�[
	mapMat->view = viewMat;
	mapMat->proj = projMat;
	constBuff->Unmap(0, nullptr);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = constBuff->GetDesc().Width;

	// �萔�o�b�t�@�r���[�쐬
	_dev->CreateConstantBufferView(
		&cbvDesc,
		basicDescHeap->GetCPUDescriptorHandleForHeapStart());

	// �����W�̍쐬
	D3D12_DESCRIPTOR_RANGE range[3]{};

	// �萔�p�i�ϊ��s��j
	range[0].NumDescriptors = 1;
	range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	range[0].BaseShaderRegister = 0;
	range[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// �萔�p�i�}�e���A���j
	range[1].NumDescriptors = 1; // ���݂͕��������A��x�Ɏg���̂�1��
	range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // �萔
	range[1].BaseShaderRegister = 1; // 1�ԃX���b�g����
	range[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// �e�N�X�`���p1��
	range[2].NumDescriptors = 4; // �ʏ�e�N�X�`���Asph�Aspa�A�g�D�[��
	range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // �萔
	range[2].BaseShaderRegister = 0; // 1�ԃX���b�g����
	range[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ���[�g�p�����[�^�[�쐬
	D3D12_ROOT_PARAMETER rootParam[2]{};
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // �V�F�[�_�[���猩����
	rootParam[0].DescriptorTable.pDescriptorRanges = &range[0]; // �f�B�X�N���v�^�����W�̃A�h���X
	rootParam[0].DescriptorTable.NumDescriptorRanges = 1; // �����W��

	// �q�[�v���قȂ�̂Ń��[�g�p�����[�^�[��2�ɕ�����
	rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // �S�ẴV�F�[�_�[���猩����
	rootParam[1].DescriptorTable.pDescriptorRanges = &range[1]; // �f�B�X�N���v�^�����W�̃A�h���X
	rootParam[1].DescriptorTable.NumDescriptorRanges = 2; // �����W��

	// �T���v���[�쐬
	D3D12_STATIC_SAMPLER_DESC samplerDesc[2]{};
	samplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // �������̌J��Ԃ�
	samplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // �c�����̌J��Ԃ�
	samplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // ���s���̌J��Ԃ�
	samplerDesc[0].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK; // �{�[�_�[�͍�
	samplerDesc[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // ���`���
	samplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX; // �~�b�v�}�b�v�ő�l
	samplerDesc[0].MinLOD = 0.0f; // �~�b�v�}�b�v�ŏ��l
	samplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // �s�N�Z���V�F�[�_�[���猩����
	samplerDesc[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // ���T���v�����O���Ȃ�
	samplerDesc[0].ShaderRegister = 0; // 0�ԃX���b�g

	// �g�D�[���p�̃T���v���[
	samplerDesc[1] = samplerDesc[0]; // �ύX�_�ȊO���R�s�[
	samplerDesc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[1].ShaderRegister = 1; // 1�ԃX���b�g

	// ���[�g�V�O�l�`���쐬
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.pParameters = rootParam; // ���[�g�p�����[�^�[�̐擪�A�h���X
	rootSignatureDesc.NumParameters = 2; // ���[�g�p�����[�^�[��
	rootSignatureDesc.pStaticSamplers = samplerDesc;
	rootSignatureDesc.NumStaticSamplers = 2;

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
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // 0�`1�ɐ��K�����ꂽRGBA

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

		// �[�x�̃N���A
		_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0.0f, 0, nullptr);

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

		// �f�B�X�N���v�^�q�[�v���Z�b�g�i�ϊ��s��p�j
		_cmdList->SetDescriptorHeaps(1, basicDescHeap.GetAddressOf());

		// ���[�g�p�����[�^�[�ƃf�B�X�N���v�^�q�[�v�̊֘A�t���i�ϊ��s��p�j
		_cmdList->SetGraphicsRootDescriptorTable(
			0, // ���[�g�p�����[�^�[�C���f�b�N�X
			basicDescHeap->GetGPUDescriptorHandleForHeapStart()); // �q�[�v�A�h���X

		auto materialH = materialDescHeap->GetGPUDescriptorHandleForHeapStart();
		unsigned int idxOffset = 0;

		// �f�B�X�N���v�^�q�[�v���Z�b�g�i�}�e���A���p�j
		_cmdList->SetDescriptorHeaps(1, materialDescHeap.GetAddressOf());

		for (auto& m : materials)
		{
			// ���[�g�p�����[�^�[�ƃf�B�X�N���v�^�q�[�v�̊֘A�t���i�}�e���A���p�j
			_cmdList->SetGraphicsRootDescriptorTable(
				1, materialH);

			// �h���[�R�[��
			_cmdList->DrawIndexedInstanced(m.indicesNum, 1, idxOffset, 0, 0);

			// �q�[�v�|�C���^�[�ƃC���f�b�N�X���ɐi�߂�
			materialH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
			idxOffset += m.indicesNum;
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

