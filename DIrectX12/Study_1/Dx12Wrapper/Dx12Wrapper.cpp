#include "Dx12Wrapper.h"

#include "../Application/Application.h"
#include "../Helper/Helper.h"

#include <d3dcompiler.h>

#pragma comment(lib,"DirectXTex.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")

HRESULT Dx12Wrapper::CreateFinalRenderTarget()
{
	DXGI_SWAP_CHAIN_DESC1 desc{};
	auto result = m_swapChain->GetDesc1(&desc);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;   // �����_�[�^�[�Q�b�g�r���[�Ȃ̂�RTV
	heapDesc.NodeMask = 0;                            // GPU�̎��ʂ̂��߂̃r�b�g�t���O�iGPU��1�����Ȃ�0�Œ�j
	heapDesc.NumDescriptors = 2;                      // �_�u���o�b�t�@�����O
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // ���Ɏw��Ȃ�

	result = m_device->CreateDescriptorHeap(&heapDesc,
		IID_PPV_ARGS(m_rtvHeap.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = m_swapChain->GetDesc(&swcDesc);
	m_backBuffers.resize(swcDesc.BufferCount);

	// �擪�̃A�h���X���擾
	D3D12_CPU_DESCRIPTOR_HANDLE handle
		= m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

	// SRGB�����_�[�^�[�Q�b�g�r���[�ݒ�
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // �K���}�␳����isRGB�j
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	// �X���b�v�`�F�[����̃o�b�N�o�b�t�@���擾
	for (int i = 0; i < swcDesc.BufferCount; ++i)
	{
		result = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i]));

		rtvDesc.Format = m_backBuffers[i]->GetDesc().Format;

		// RTV�̍쐬
		m_device->CreateRenderTargetView(
			m_backBuffers[i],
			&rtvDesc,
			handle // �f�B�X�N���v�^�q�[�v�n���h��
		);

		// ���ւ��炷
		handle.ptr += m_device->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	m_viewport.reset(new CD3DX12_VIEWPORT(m_backBuffers[0]));
	m_scissorRect.reset(new CD3DX12_RECT(0, 0, desc.Width, desc.Height));

	return result;
}

HRESULT Dx12Wrapper::CreateDepthStencilView()
{
	auto depthResDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R32_TYPELESS,
		m_windowSize.cx,
		m_windowSize.cy,
		1,
		1,
		1,
		0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	auto depthHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	CD3DX12_CLEAR_VALUE depthClearValue(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);

	auto result = m_device->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, // �[�x�l�������݂Ɏg�p
		&depthClearValue,
		IID_PPV_ARGS(m_depthBuffer.GetAddressOf())
	);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// �[�x�̂��߂̃f�B�X�N���v�^�q�[�v�����
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {}; // �[�x�Ɏg�����Ƃ��킩��΂悢
	dsvHeapDesc.NumDescriptors = 1; // �[�x�r���[��1��
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV; // �f�v�X�X�e���V���r���[�Ƃ��Ďg��
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	result = m_device->CreateDescriptorHeap(
		&dsvHeapDesc,
		IID_PPV_ARGS(m_dsvHeap.ReleaseAndGetAddressOf())
	);

	// �[�x�r���[�쐬
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // �[�x�l��32�r�b�g�g�p
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2D�e�N�X�`��
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE; // �t���O�͓��ɂȂ�

	m_device->CreateDepthStencilView(
		m_depthBuffer.Get(),
		&dsvDesc,
		m_dsvHeap->GetCPUDescriptorHandleForHeapStart()
	);

	return result;
}

HRESULT Dx12Wrapper::CreateSwapChain(const HWND & hwnd)
{
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};

	swapChainDesc.Width = m_windowSize.cx; // ��ʉ𑜓x�y���z
	swapChainDesc.Height = m_windowSize.cy; // ��ʉ𑜓x�y���z
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // �s�N�Z���t�H�[�}�b�g
	swapChainDesc.Stereo = false; // �X�e���I�\���t���O�i3D�f�B�X�v���C�̃X�e���I���[�h�j
	swapChainDesc.SampleDesc.Count = 1; // �}���`�T���v���̎w��i1�ł悢�j
	swapChainDesc.SampleDesc.Quality = 0; // �}���`�T���v���̎w��i0�ł悢�j
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER; // ����ł悢
	swapChainDesc.BufferCount = 2; // �_�u���o�b�t�@�Ȃ�2�ł悢

	// �o�b�N�o�b�t�@�[�͐L�яk�݉\
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;

	// �t���b�v��͑��₩�ɔj��
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	// ���Ɏw��Ȃ�
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

	// �E�B���h�E�̃t���X�N���[���؂�ւ��\
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	return m_factory->CreateSwapChainForHwnd(
		m_cmdQueue.Get(),
		hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)m_swapChain.ReleaseAndGetAddressOf());
}

HRESULT Dx12Wrapper::InitializeDXGIDevice()
{
	UINT flagsDXGI = 0 | DXGI_CREATE_FACTORY_DEBUG;

	auto result = CreateDXGIFactory2(flagsDXGI, IID_PPV_ARGS(m_factory.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	std::vector<IDXGIAdapter*> adapters; // �A�_�v�^�[�̗񋓗p
	IDXGIAdapter* tmpAdapter{ nullptr };

	for (int i = 0; m_factory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(tmpAdapter);
	}

	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc); // �A�_�v�^�[�̐����I�u�W�F�N�g�擾

		std::wstring strDesc = adesc.Description;

		// �T�������A�_�v�^�[�̖��O���m�F
		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}

	// �����\�ȃt�B�[�`���[���x���̌����Ɏg��
	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	//D3D_FEATURE_LEVEL featureLevel;

	for (auto fl : levels)
	{
		if (SUCCEEDED(D3D12CreateDevice(tmpAdapter, fl, IID_PPV_ARGS(m_device.ReleaseAndGetAddressOf()))))
		{
			//featureLevel = fl;
			result = S_OK;
			break; // �����\�ȃo�[�W���������������烋�[�v�I��
		}
	}

	return result;
}

HRESULT Dx12Wrapper::InitializeCommand()
{
	auto result = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cmdAllocator));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	result = m_device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_cmdAllocator.Get(),
		nullptr,
		IID_PPV_ARGS(&m_cmdList));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};

	// �^�C���A�E�g�Ȃ�
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	// �A�_�v�^�[��1�����g��Ȃ��Ƃ���0�ł悢
	cmdQueueDesc.NodeMask = 0;

	// �v���C�I���e�B�͓��Ɏw��Ȃ�
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

	// �R�}���h���X�g�ƍ��킹��
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	result = m_device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&m_cmdQueue));

	return result;
}

HRESULT Dx12Wrapper::CreateSceneView()
{
	DXGI_SWAP_CHAIN_DESC1 desc{};
	auto result = m_swapChain->GetDesc1(&desc);

	// �萔�o�b�t�@�쐬
	result = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneData) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_sceneConstBuff.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	m_mappedSceneData = nullptr;
	result = m_sceneConstBuff->Map(0, nullptr, (void**)&m_mappedSceneData);

	SetCameraSetting();

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // �V�F�[�_�[���猩����悤��
	descHeapDesc.NodeMask = 0; // �}�X�N�� 0
	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; // �q�[�v���

	result = m_device->CreateDescriptorHeap(&descHeapDesc,
		IID_PPV_ARGS(m_sceneDescHeap.ReleaseAndGetAddressOf())); // ����

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = m_sceneConstBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = m_sceneConstBuff->GetDesc().Width;

	// �萔�o�b�t�@�[�r���[�̍쐬
	auto basicHeapHandle = m_sceneDescHeap->GetCPUDescriptorHandleForHeapStart();
	m_device->CreateConstantBufferView(
		&cbvDesc, basicHeapHandle
	);

	return result;
}

void Dx12Wrapper::CreateTextureLoaderTable()
{
	m_loadLambdaTable["sph"] = m_loadLambdaTable["spa"] = m_loadLambdaTable["bmp"] = m_loadLambdaTable["png"] = m_loadLambdaTable["jpg"] = [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT {
		return LoadFromWICFile(path.c_str(), 0, meta, img);
	};


	m_loadLambdaTable["tga"] // TGA�Ȃǂ̈ꕔ�� 3D �\�t�g�Ŏg�p����Ă���e�N�X�`���t�@�C���`��
		= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)
		->HRESULT
	{
		return LoadFromTGAFile(path.c_str(), meta, img);
	};

	m_loadLambdaTable["dds"] // DirectX�p�̈��k�e�N�X�`���t�@�C���`��
		= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)
		->HRESULT
	{
		return LoadFromDDSFile(path.c_str(), 0, meta, img);
	};
}

ID3D12Resource * Dx12Wrapper::CreateTextureFromFile(const char * texPath)
{
	// WIC�e�N�X�`���̃��[�h
	TexMetadata metadata{};
	ScratchImage scratchImg{};

	auto wtexpath = GetWideStringFromString(texPath); // �e�N�X�`���̃t�@�C���p�X

	auto ext = GetExtension(texPath); // �g���q���擾

	auto result = m_loadLambdaTable[ext](
		wtexpath,
		&metadata,
		scratchImg
		);

	if (FAILED(result))
	{
		return nullptr;
	}

	auto img = scratchImg.GetImage(0, 0, 0); // ���f�[�^���o

	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		metadata.format,
		metadata.width,
		metadata.height,
		metadata.arraySize,
		metadata.mipLevels);

	// �o�b�t�@�쐬
	ID3D12Resource* texBuff{ nullptr };

	result = m_device->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE, // ���Ɏw��Ȃ�
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&texBuff)
	);

	if (FAILED(result))
	{
		return nullptr;
	}

	result = texBuff->WriteToSubresource(
		0,
		nullptr, // �S�̈�փR�s�[
		img->pixels, // ���f�[�^�A�h���X
		img->rowPitch, // 1���C���T�C�Y
		img->slicePitch // �S�T�C�Y
	);

	if (FAILED(result))
	{
		return nullptr;
	}

	return texBuff;
}

ID3D12Resource * Dx12Wrapper::CreateWhiteTexture()
{
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4);

	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	ID3D12Resource* whiteBuff{ nullptr };

	auto result = m_device->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE, // ���Ɏw��Ȃ�
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&whiteBuff));

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0xff); // �S�� 255 �Ŗ��߂�

	// �f�[�^�]��
	result = whiteBuff->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * 4,
		data.size());

	if (FAILED(result))
	{
		return nullptr;
	}

	return whiteBuff;
}

ID3D12Resource * Dx12Wrapper::CreateBlackTexture()
{
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4);

	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	ID3D12Resource* blackBuff{ nullptr };

	auto result = m_device->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE, // ���Ɏw��Ȃ�
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&blackBuff));

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0x00); // �S�� 0 �Ŗ��߂�

	// �f�[�^�]��
	result = blackBuff->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * 4,
		data.size());

	if (FAILED(result))
	{
		return nullptr;
	}

	return blackBuff;
}

ID3D12Resource * Dx12Wrapper::CreateGrayGradationTexture()
{
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, 256);

	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	ID3D12Resource* gradBuff{ nullptr };

	auto result = m_device->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE, // ���Ɏw��Ȃ�
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&gradBuff));

	// �オ�����Ă����������e�N�X�`���f�[�^���쐬
	std::vector<unsigned int> data(4 * 256);
	auto it = data.begin();
	unsigned int c = 0xff;

	for (; it != data.end(); it += 4)
	{
		auto col = (c << 0xff) | (c << 16) | (c << 8) | c;
		std::fill(it, it + 4, col);
		--c;
	}

	// �f�[�^�]��
	result = gradBuff->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * sizeof(unsigned int),
		sizeof(unsigned int) * data.size());

	if (FAILED(result))
	{
		return nullptr;
	}

	return gradBuff;
}

bool Dx12Wrapper::CreatePeraResourceAndView()
{
	// ==========�������ݐ惊�\�[�X�̍쐬==========

	// �쐬�ς݂̃q�[�v�����g���Ă���1�����
	auto heapDesc = m_rtvHeap->GetDesc();

	// �g���Ă���o�b�N�o�b�t�@�[�̏��𗘗p����
	auto& bbuff = m_backBuffers[0];
	auto resDesc = bbuff->GetDesc();

	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	// �����_�����O���̃N���A�l�Ɠ����l
	D3D12_CLEAR_VALUE clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, m_clearColor);

	// 1����
	auto result = m_device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(m_peraResource.ReleaseAndGetAddressOf()));

	// 2����
	result = m_device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(m_peraResource2.ReleaseAndGetAddressOf()));

	// ==========�r���[�iRTV/SRV�j�����==========

	//�@RTV�p�q�[�v�����
	heapDesc.NumDescriptors = 2;
	result = m_device->CreateDescriptorHeap(
		&heapDesc,
		IID_PPV_ARGS(m_peraRTVHeap.ReleaseAndGetAddressOf()));

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	auto handle = m_peraRTVHeap->GetCPUDescriptorHandleForHeapStart();
	// �����_�[�^�[�Q�b�g�r���[�iRTV�j�����
	m_device->CreateRenderTargetView(
		m_peraResource.Get(),
		&rtvDesc,
		handle);

	handle.ptr += m_device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_device->CreateRenderTargetView(
		m_peraResource2.Get(),
		&rtvDesc,
		handle);

	// SRV�p�q�[�v�����
	heapDesc.NumDescriptors = 3; // �K�E�X�p�����[�^�[�ƃV�F�[�_�[2��
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;

	result = m_device->CreateDescriptorHeap(
		&heapDesc,
		IID_PPV_ARGS(m_peraSRVHeap.ReleaseAndGetAddressOf()));

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = rtvDesc.Format;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	handle = m_peraSRVHeap->GetCPUDescriptorHandleForHeapStart();

	// �V�F�[�_�[���\�[�X�r���[�iSRV�j�����i1���ځj
	m_device->CreateShaderResourceView(
		m_peraResource.Get(),
		&srvDesc,
		handle);

	handle.ptr += m_device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_device->CreateShaderResourceView(
		m_peraResource2.Get(),
		&srvDesc,
		handle);

	// ==========�{�P�p�̒萔�o�b�t�@�r���[�����==========
	handle.ptr += m_device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	cbvDesc.BufferLocation = m_bokehParamBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = m_bokehParamBuffer->GetDesc().Width;

	m_device->CreateConstantBufferView(
		&cbvDesc, handle);

	return true;
}

bool Dx12Wrapper::CreatePeraPolygon()
{
	struct PeraVertex
	{
		XMFLOAT3 pos;
		XMFLOAT2 uv;
	};

	PeraVertex pv[4] =
	{
		{ { -1, -1, 0.1f }, { 0, 1 } },
		{ { -1,  1, 0.1f }, { 0, 0 } },
		{ {  1, -1, 0.1f }, { 1, 1 } },
		{ {  1,  1, 0.1f }, { 1, 0 } },
	};

	// ���_�o�b�t�@�[���쐬
	auto result = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(pv)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_peraVB.ReleaseAndGetAddressOf()));

	m_peraVBV.BufferLocation = m_peraVB->GetGPUVirtualAddress();
	m_peraVBV.SizeInBytes = sizeof(pv);
	m_peraVBV.StrideInBytes = sizeof(PeraVertex);

	PeraVertex* mappedPera{ nullptr };
	m_peraVB->Map(0, nullptr, (void**)&mappedPera);
	std::copy(std::begin(pv), std::end(pv), mappedPera);
	m_peraVB->Unmap(0, nullptr);

	D3D12_INPUT_ELEMENT_DESC layout[2] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	ComPtr<ID3DBlob> vs;
	ComPtr<ID3DBlob> ps;
	ComPtr<ID3DBlob> errBlob;

	result = D3DCompileFromFile(L"peraVertex.hlsl", nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, "vs", "vs_5_0", 0, 0,
		vs.ReleaseAndGetAddressOf(), errBlob.ReleaseAndGetAddressOf());

	result = D3DCompileFromFile(L"peraPixel.hlsl", nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, "ps", "ps_5_0", 0, 0,
		ps.ReleaseAndGetAddressOf(), errBlob.ReleaseAndGetAddressOf());

	D3D12_DESCRIPTOR_RANGE range[4]{};
	range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[0].BaseShaderRegister = 0;
	range[0].NumDescriptors = 1;

	range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	range[1].BaseShaderRegister = 0;
	range[1].NumDescriptors = 1;

	range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[2].BaseShaderRegister = 1;
	range[2].NumDescriptors = 1;

	range[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[3].BaseShaderRegister = 2;
	range[3].NumDescriptors = 1; // �g���̂�1�it2)...2?

	D3D12_ROOT_PARAMETER rp[4]{};
	rp[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rp[0].DescriptorTable.pDescriptorRanges = &range[0];
	rp[0].DescriptorTable.NumDescriptorRanges = 1;

	rp[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rp[1].DescriptorTable.pDescriptorRanges = &range[1];
	rp[1].DescriptorTable.NumDescriptorRanges = 1;

	rp[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rp[2].DescriptorTable.pDescriptorRanges = &range[2];
	rp[2].DescriptorTable.NumDescriptorRanges = 1;

	rp[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rp[3].DescriptorTable.pDescriptorRanges = &range[3];
	rp[3].DescriptorTable.NumDescriptorRanges = 1;

	D3D12_STATIC_SAMPLER_DESC sampler = CD3DX12_STATIC_SAMPLER_DESC(0);
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

	D3D12_ROOT_SIGNATURE_DESC rsDesc{};
	rsDesc.NumParameters = 4;
	rsDesc.pParameters = rp;
	rsDesc.NumStaticSamplers = 1;
	rsDesc.pStaticSamplers = &sampler;
	rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> rsBlob;

	result = D3D12SerializeRootSignature(
		&rsDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		rsBlob.ReleaseAndGetAddressOf(),
		errBlob.ReleaseAndGetAddressOf());

	result = m_device->CreateRootSignature(
		0,
		rsBlob->GetBufferPointer(),
		rsBlob->GetBufferSize(),
		IID_PPV_ARGS(m_peraRS.ReleaseAndGetAddressOf()));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc{};
	gpsDesc.InputLayout.NumElements = _countof(layout);
	gpsDesc.InputLayout.pInputElementDescs = layout;
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(vs.Get());
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	gpsDesc.NumRenderTargets = 1;
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.SampleDesc.Quality = 0;
	gpsDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	gpsDesc.pRootSignature = m_peraRS.Get();

	// 1���ڗp�p�C�v���C������
	result = m_device->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(m_peraPipeline.ReleaseAndGetAddressOf()));

	// 2���ڗp�s�N�Z���V�F�[�_�[
	result = D3DCompileFromFile(L"pera.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VerticalBokehPS", "ps_5_0", 0, 0,
		ps.ReleaseAndGetAddressOf(), errBlob.ReleaseAndGetAddressOf());

	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(ps.Get());

	// 2���ڗp�p�C�v���C������
	result = m_device->CreateGraphicsPipelineState(
		&gpsDesc, IID_PPV_ARGS(m_peraPipeline2.ReleaseAndGetAddressOf()));

	return true;
}

void Dx12Wrapper::CreateBokehParamResource()
{
	auto weights = GetGaussianWeights(8, 5.0f);
	auto result = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(
			AlignmentedSize(sizeof(weights[0]) * weights.size(), 256)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_bokehParamBuffer.ReleaseAndGetAddressOf()));

	float* mappedWeight{ nullptr };
	result = m_bokehParamBuffer->Map(0, nullptr, (void**)&mappedWeight);
	copy(weights.begin(), weights.end(), mappedWeight);
	m_bokehParamBuffer->Unmap(0, nullptr);
}

bool Dx12Wrapper::CreateEffectBufferAndView()
{
	// �@���}�b�v�����[�h����
	m_effectTexBuffer = GetTextureByPath("normal/normalmap.jpg");

	if (m_effectTexBuffer == nullptr)
	{
		return false;
	}

	// �|�X�g�G�t�F�N�g�p�̃f�B�X�N���v�^�q�[�v����
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NumDescriptors = 1;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	auto result = m_device->CreateDescriptorHeap(
		&heapDesc, IID_PPV_ARGS(m_effectSRVHeap.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return false;
	}

	// �|�X�g�G�t�F�N�g�p�e�N�X�`���r���[�����
	auto desc = m_effectTexBuffer->GetDesc();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = desc.Format;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	m_device->CreateShaderResourceView(
		m_effectTexBuffer.Get(),
		&srvDesc,
		m_effectSRVHeap->GetCPUDescriptorHandleForHeapStart());

	return true;
}

bool Dx12Wrapper::CreateDepthSRV()
{
	// �q�[�v�쐬
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 1;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	
	auto result = m_device->CreateDescriptorHeap(
		&heapDesc, IID_PPV_ARGS(m_depthSRVHeap.ReleaseAndGetAddressOf()));

	// SRV�쐬
	D3D12_SHADER_RESOURCE_VIEW_DESC resDesc{};
	resDesc.Format = DXGI_FORMAT_R32_FLOAT;
	resDesc.Texture2D.MipLevels = 1;
	resDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	resDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

	auto handle = m_depthSRVHeap->GetCPUDescriptorHandleForHeapStart();

	m_device->CreateShaderResourceView(
		m_depthBuffer.Get(), &resDesc, handle);

	return true;
}

Dx12Wrapper::Dx12Wrapper(HWND hwnd)
	: m_eye(0, 15, -25)
	, m_target(0, 10, 0)
	, m_up(0, 1, 0)
{
#ifdef _DEBUG
	//�f�o�b�O���C���[���I����
	EnableDebugLayer();
#endif

	auto& app = Application::Instance();
	m_windowSize = app.GetWindowSize();
	m_parallelLightVec = { 1.0f, -1.0f, 1.0f };

	//DirectX12�֘A������
	if (FAILED(InitializeDXGIDevice()))
	{
		assert(0);
		return;
	}
	if (FAILED(InitializeCommand()))
	{
		assert(0);
		return;
	}
	if (FAILED(CreateSwapChain(hwnd)))
	{
		assert(0);
		return;
	}
	if (FAILED(CreateFinalRenderTarget()))
	{
		assert(0);
		return;
	}

	if (FAILED(CreateSceneView()))
	{
		assert(0);
		return;
	}

	//�e�N�X�`�����[�_�[�֘A������
	CreateTextureLoaderTable();

	//�[�x�o�b�t�@�쐬
	if (FAILED(CreateDepthStencilView()))
	{
		assert(0);
		return;
	}

	CreateDepthSRV();

	//�t�F���X�̍쐬
	if (FAILED(m_device->CreateFence(m_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.ReleaseAndGetAddressOf()))))
	{
		assert(0);
		return;
	}

	m_whiteTex = CreateWhiteTexture();
	m_blackTex = CreateBlackTexture();
	m_gradTex = CreateGrayGradationTexture();

	CreateBokehParamResource();
	CreateEffectBufferAndView();
	CreatePeraResourceAndView();
	CreatePeraPolygon();
}

Dx12Wrapper::~Dx12Wrapper()
{
}

void Dx12Wrapper::Update()
{
}

void Dx12Wrapper::BeginDraw()
{
	auto bbIdx = m_swapChain->GetCurrentBackBufferIndex();

	auto rtvH = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += bbIdx * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// CD3DX�`���g�����o���A�̐ݒ�
	m_cmdList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_backBuffers[bbIdx],
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET));

	auto dsvH = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

	m_cmdList->OMSetRenderTargets(
		1,      // �����_�[�^�[�Q�b�g�̐�
		&rtvH,  // �����_�[�^�[�Q�b�g�n���h���̐擪�A�h���X
		false,   // �������ɘA�����Ă��邩
		&dsvH // �[�x�X�e���V���o�b�t�@�[�r���[�̃n���h��
	);

	// ��ʃN���A
	m_cmdList->ClearRenderTargetView(
		rtvH,
		m_clearColor,
		0,      // �S��ʃN���A�Ȃ�w�肷��K�v�Ȃ�
		nullptr // �S��ʃN���A�Ȃ�w�肷��K�v�Ȃ�
	);

	m_cmdList->ClearDepthStencilView(
		dsvH,
		D3D12_CLEAR_FLAG_DEPTH,
		1.0f,
		0,
		0,
		nullptr);

	m_cmdList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_peraResource.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
}

void Dx12Wrapper::PreDrawToPera1()
{
	m_cmdList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_peraResource.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET));

	auto rtvHeapPointer = m_peraRTVHeap->GetCPUDescriptorHandleForHeapStart();
	auto dsvHeapPointer = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

	m_cmdList->OMSetRenderTargets(1, &rtvHeapPointer, false, &dsvHeapPointer);
	m_cmdList->ClearRenderTargetView(rtvHeapPointer, m_clearColor, 0, nullptr);
	m_cmdList->ClearDepthStencilView(dsvHeapPointer, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void Dx12Wrapper::SetCameraInfoToConstBuff()
{
	// �r���[�s��Ƃ��̃Z�b�g�����Ă�B
	// �܂�A�J���������Z�b�g���Ă�B

	ID3D12DescriptorHeap* sceneHeaps[] = { m_sceneDescHeap.Get() };

	m_cmdList->SetDescriptorHeaps(1, sceneHeaps);
	m_cmdList->SetGraphicsRootDescriptorTable(0, m_sceneDescHeap->GetGPUDescriptorHandleForHeapStart());

	// �r���[�|�[�g���Z�b�g
	m_cmdList->RSSetViewports(1, m_viewport.get());

	// �V�U�[��`���Z�b�g
	m_cmdList->RSSetScissorRects(1, m_scissorRect.get());
}

void Dx12Wrapper::SetCameraSetting()
{
	auto eyePos = XMLoadFloat3(&m_eye);
	auto targetPos = XMLoadFloat3(&m_target);
	auto upVec = XMLoadFloat3(&m_up);

	m_mappedSceneData->eye = m_eye;
	m_mappedSceneData->view = XMMatrixLookAtLH(eyePos, targetPos, upVec);
	m_mappedSceneData->proj = XMMatrixPerspectiveFovLH(
		XM_PIDIV4,
		static_cast<float>(m_windowSize.cx) / static_cast<float>(m_windowSize.cy),
		0.1f,
		100.0f);

	auto plane = XMFLOAT4(0, 1, 0, 0); // ����
	XMVECTOR planeVec = XMLoadFloat4(&plane);

	auto light = XMFLOAT4(-1, 1, -1, 0);
	XMVECTOR lightVec = XMLoadFloat4(&light);
	m_mappedSceneData->shadow = XMMatrixShadow(planeVec, lightVec);

	auto lightPos = targetPos + XMVector3Normalize(lightVec)
		* XMVector3Length(XMVectorSubtract(targetPos, eyePos)).m128_f32[0];

	m_mappedSceneData->lightCamera =
		XMMatrixLookAtLH(lightPos, targetPos, upVec)
		* XMMatrixOrthographicLH(40, 40, 0.1f, 100.0f);
}

void Dx12Wrapper::DrawHorizontalBokeh()
{
	m_cmdList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_peraResource.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	// 2���ڂ̃��\�[�X�̏�Ԃ������_�[�^�[�Q�b�g�ɑJ�ڂ�����
	m_cmdList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_peraResource2.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET));

	auto rtvHeapPointer =
		m_peraRTVHeap->GetCPUDescriptorHandleForHeapStart();

	// 2����RTV�Ȃ̂Ői�܂��ă����_�[�^�[�Q�b�g�����蓖�Ă�
	rtvHeapPointer.ptr += m_device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	m_cmdList->OMSetRenderTargets(1, &rtvHeapPointer, false, nullptr);
	m_cmdList->ClearRenderTargetView(rtvHeapPointer, m_clearColor, 0, nullptr);

	m_cmdList->SetGraphicsRootSignature(m_peraRS.Get());
	m_cmdList->SetDescriptorHeaps(1, m_peraSRVHeap.GetAddressOf());

	auto handle = m_peraSRVHeap->GetGPUDescriptorHandleForHeapStart();
	m_cmdList->SetGraphicsRootDescriptorTable(0, handle);

	handle.ptr += m_device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_cmdList->SetGraphicsRootDescriptorTable(1, handle);

	m_cmdList->SetPipelineState(m_peraPipeline.Get());
	m_cmdList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	m_cmdList->IASetVertexBuffers(0, 1, &m_peraVBV);
	m_cmdList->DrawInstanced(4, 1, 0, 0);

	m_cmdList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_peraResource2.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
}

void Dx12Wrapper::Clear()
{
	auto bbIdx = m_swapChain->GetCurrentBackBufferIndex();

	m_cmdList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_backBuffers[bbIdx],
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET));

	auto rtvH = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += bbIdx * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	m_cmdList->OMSetRenderTargets(
		1,      // �����_�[�^�[�Q�b�g�̐�
		&rtvH,  // �����_�[�^�[�Q�b�g�n���h���̐擪�A�h���X
		false,   // �������ɘA�����Ă��邩
		nullptr // �[�x�X�e���V���o�b�t�@�[�r���[�̃n���h��
	);

	m_cmdList->ClearRenderTargetView(rtvH, m_clearColor, 0, nullptr);
}

void Dx12Wrapper::Draw()
{
	m_cmdList->SetGraphicsRootSignature(m_peraRS.Get());

	// �܂��q�[�v���Z�b�g
	m_cmdList->SetDescriptorHeaps(1, m_peraSRVHeap.GetAddressOf());
	auto handle = m_peraSRVHeap->GetGPUDescriptorHandleForHeapStart();

	// �p�����[�^�[0�Ԃƃq�[�v���֘A�t����
	m_cmdList->SetGraphicsRootDescriptorTable(0, handle);

	handle.ptr += m_device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	handle.ptr += m_device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_cmdList->SetGraphicsRootDescriptorTable(1, handle);

	// �[�x�o�b�t�@�e�N�X�`��
	m_cmdList->SetDescriptorHeaps(1, m_depthSRVHeap.GetAddressOf());
	m_cmdList->SetGraphicsRootDescriptorTable(3, 
		m_depthSRVHeap->GetGPUDescriptorHandleForHeapStart());

	m_cmdList->SetPipelineState(m_peraPipeline.Get());
	m_cmdList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	m_cmdList->IASetVertexBuffers(0, 1, &m_peraVBV);
	m_cmdList->SetDescriptorHeaps(1, m_effectSRVHeap.GetAddressOf());
	m_cmdList->SetGraphicsRootDescriptorTable(2, 
		m_effectSRVHeap->GetGPUDescriptorHandleForHeapStart());
	m_cmdList->DrawInstanced(4, 1, 0, 0);
}

void Dx12Wrapper::Flip()
{
	m_cmdList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_backBuffers[m_swapChain->GetCurrentBackBufferIndex()],
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT));

	// ���߂̃N���[�Y
	m_cmdList->Close();

	// �R�}���h���X�g�̎��s
	ID3D12CommandList* cmdLists[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(
		1,       // ���s����R�}���h���X�g�̐�
		cmdLists // �R�}���h���X�g�̔z��̐擪�A�h���X
	);

	m_cmdQueue->Signal(m_fence.Get(), ++m_fenceVal);

	if (m_fence->GetCompletedValue() < m_fenceVal)
	{
		// �C�x���g�n���h���̎擾
		auto event = CreateEvent(nullptr, false, false, nullptr);

		m_fence->SetEventOnCompletion(m_fenceVal, event);

		// �C�x���g����������܂ő҂�������iINFINITE�j
		WaitForSingleObject(event, INFINITE);

		// �C�x���g�n���h�������
		CloseHandle(event);
	}

	m_cmdAllocator->Reset(); // �L���[���N���A�i�Ȃ��u�L���[�v�̃N���A�H�j
	m_cmdList->Reset(m_cmdAllocator.Get(), nullptr); // �ĂуR�}���h���X�g�����߂鏀��
	m_swapChain->Present(0, 0);
}

ComPtr<ID3D12Resource> Dx12Wrapper::GetTextureByPath(const char * texPath)
{
	auto it = m_textureTable.find(texPath);

	if (it != m_textureTable.end())
	{
		// �e�[�u�����ɂ������烍�[�h����̂ł͂Ȃ�
		// �}�b�v���̃��\�[�X��Ԃ�
		return m_textureTable[texPath];
	}
	else
	{
		return ComPtr<ID3D12Resource>(CreateTextureFromFile(texPath));
	}
}

ComPtr<ID3D12Device> Dx12Wrapper::Device()
{
	return m_device;
}

ComPtr<ID3D12GraphicsCommandList> Dx12Wrapper::CommandList()
{
	return m_cmdList;
}

ComPtr<IDXGISwapChain4> Dx12Wrapper::Swapchain()
{
	return m_swapChain;
}

ComPtr<ID3D12Resource> Dx12Wrapper::WhiteTexture() const
{
	return m_whiteTex;
}

ComPtr<ID3D12Resource> Dx12Wrapper::BlackTexture() const
{
	return m_blackTex;
}

ComPtr<ID3D12Resource> Dx12Wrapper::GradTexture() const
{
	return m_gradTex;
}
