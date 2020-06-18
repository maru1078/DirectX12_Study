#include "Dx12Wrapper.h"

#include "../Application/Application.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "DirectXTex.lib")

void EnableDebugLayer()
{
#ifdef _DEBUG

	ID3D12Debug* debugLayer{ nullptr };
	auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));

	debugLayer->EnableDebugLayer(); // �f�o�b�O���C���[��L��������
	debugLayer->Release(); // �L����������C���^�[�t�F�C�X���������

#endif // _DEBUG
}

Dx12Wrapper::Dx12Wrapper()
{
	EnableDebugLayer();

	if (!InitializeDXGIDevice())
	{
		assert(0);
	}
	if (!InitializeCommandList())
	{
		assert(0);
	}
	if (!CreateSwapChain())
	{
		assert(0);
	}
	if (!CreateFence())
	{
		assert(0);
	}
	if (!CreateFinalRenderTargets())
	{
		assert(0);
	}
	if (!CreateDepthStencilView())
	{
		assert(0);
	}
	if (!CreateSceneView())
	{
		assert(0);
	}
	if (!CreateWhiteTexture())
	{
		assert(0);
	}
	if (!CreateBlackTexture())
	{
		assert(0);
	}
	if (!CreateGrayGradTexture())
	{
		assert(0);
	}

	CreateTextureLoaderTable();
}

// +-------------------------------+ //
// |          public�֐�           | //
// +-------------------------------+ //

ComPtr<ID3D12Device> Dx12Wrapper::Device() const
{
	return m_device;
}

ComPtr<ID3D12GraphicsCommandList> Dx12Wrapper::CommandList() const
{
	return m_cmdList;
}

const ComPtr<ID3D12Resource> Dx12Wrapper::WhiteTex() const
{
	return m_whiteTex;
}

const ComPtr<ID3D12Resource> Dx12Wrapper::BlackTex() const
{
	return m_blackTex;
}

const ComPtr<ID3D12Resource> Dx12Wrapper::GrayGradTex() const
{
	return m_gradTex;
}

void Dx12Wrapper::BeginDraw()
{
	// ���݂̃o�b�N�o�b�t�@�[�̃C���f�b�N�X���擾
    m_bbIdx = m_swapChain->GetCurrentBackBufferIndex();

	m_cmdList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_backBuffers[m_bbIdx].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// �����_�[�^�[�Q�b�g�r���[�A�f�v�X�X�e���V���r���[���Z�b�g
	auto rtvH = m_rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += m_bbIdx * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	auto dsvH = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

	m_cmdList->OMSetRenderTargets(1, &rtvH, false, &dsvH);

	// �����_�[�^�[�Q�b�g�̃N���A
	m_cmdList->ClearRenderTargetView(rtvH, m_clearColor, 0, nullptr);

	// �[�x�̃N���A
	m_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0.0f, 0, nullptr);

	// �r���[�|�[�g���Z�b�g
	m_cmdList->RSSetViewports(1, &m_viewport);

	// �V�U�[��`���Z�b�g
	m_cmdList->RSSetScissorRects(1, &m_scissorRect);
}

void Dx12Wrapper::SetSceneMat()
{
	// �f�B�X�N���v�^�q�[�v���Z�b�g�i�ϊ��s��p�j
	m_cmdList->SetDescriptorHeaps(1, m_basicDescHeap.GetAddressOf());

	// ���[�g�p�����[�^�[�ƃf�B�X�N���v�^�q�[�v�̊֘A�t���i�ϊ��s��p�j
	m_cmdList->SetGraphicsRootDescriptorTable(
		0, // ���[�g�p�����[�^�[�C���f�b�N�X
		m_basicDescHeap->GetGPUDescriptorHandleForHeapStart()); // �q�[�v�A�h���X
}

void Dx12Wrapper::EndDraw()
{
	m_cmdList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_backBuffers[m_bbIdx].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// �R�}���h���X�g���s
	m_cmdList->Close();

	ID3D12CommandList* cmdLists[]{ m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(1, cmdLists);

	WaitForCommandQueue();

	// �R�}���h���X�g�Ȃǂ̃N���A
	m_cmdAllocator->Reset(); // �L���[�̃N���A
	m_cmdList->Reset(m_cmdAllocator.Get(), nullptr);

	// �t���b�v
	m_swapChain->Present(1, 0);
}

void Dx12Wrapper::WaitForCommandQueue()
{
	// ��������
	m_cmdQueue->Signal(m_fence.Get(), ++m_fenceVal);
	while (m_fence->GetCompletedValue() != m_fenceVal)
	{
		// �C�x���g�n���h���̎擾
		auto event = CreateEvent(nullptr, false, false, nullptr);

		m_fence->SetEventOnCompletion(m_fenceVal, event);

		// �C�x���g����������܂ő҂�������iINFINITE�j
		WaitForSingleObject(event, INFINITE);

		// �C�x���g�n���h�������
		CloseHandle(event);
	}
}

ComPtr<ID3D12Resource> Dx12Wrapper::GetTextureByPath(const std::string & path)
{
	auto it = m_resourceTable.find(path);

	// �e�[�u�����ɂ������烍�[�h����̂ł͂Ȃ�
	// �}�b�v���̃��\�[�X��Ԃ�
	if (it != m_resourceTable.end())
	{
		return it->second;
	}

	return LoadTextureFromFile(path);
}

void Dx12Wrapper::UpdateWorldMat()
{
	angle += 0.01f;
	//m_mapMat->world = XMMatrixRotationY(angle);
}

// +-------------------------------+ //
// |          private�֐�          | //
// +-------------------------------+ //

bool Dx12Wrapper::InitializeDXGIDevice()
{
#ifdef _DEBUG

	auto result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, 
		IID_PPV_ARGS(m_dxgiFactory.ReleaseAndGetAddressOf()));

#else

	auto result = CreateDXGIFactory1(IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf()));

#endif // _DEBUG

	if (FAILED(result)) return false;

	std::vector<ComPtr<IDXGIAdapter>> adapters;
	ComPtr<IDXGIAdapter> tmpAdapter{ nullptr };

	for (int i = 0; m_dxgiFactory->EnumAdapters(i, tmpAdapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++i)
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

	//D3D_FEATURE_LEVEL featureLevel;

	for (auto lv : levels)
	{
		// ��1������nullptr�ɂ���ƃ_���B
		// Intel(R) UHD Graphics 630�ł����܂������Ȃ��B���f����������
		// ����� "NVIDIA" �����A�_�v�^�[���w��B
		if (D3D12CreateDevice(tmpAdapter.Get(), lv, IID_PPV_ARGS(m_device.ReleaseAndGetAddressOf())) == S_OK)
		{
			//featureLevel = lv;
			break; // �����\�ȃo�[�W���������������烋�[�v��ł��؂�
		}
	}

	return true;
}

bool Dx12Wrapper::InitializeCommandList()
{
	// �R�}���h�A���P�[�^�[�쐬
	auto result = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(m_cmdAllocator.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	// �R�}���h���X�g�쐬
	result = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, 
		m_cmdAllocator.Get(), nullptr, IID_PPV_ARGS(m_cmdList.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	// �R�}���h�L���[�쐬
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc{};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; // �t���O�Ȃ�
	cmdQueueDesc.NodeMask = 0; // �A�_�v�^�[��1�����g��Ȃ��Ƃ���0�ł悢
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL; // �v���C�I���e�B�͓��Ɏw��Ȃ�
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // �R�}���h���X�g�ƍ��킹��
	result = m_device->CreateCommandQueue(&cmdQueueDesc,
		IID_PPV_ARGS(m_cmdQueue.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	return true;
}

bool Dx12Wrapper::CreateSwapChain()
{
	// �X���b�v�`�F�[���쐬
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = Application::Instance().WindowWidth();
	swapChainDesc.Height = Application::Instance().WindowHeight();
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
	auto result = m_dxgiFactory->CreateSwapChainForHwnd(
		m_cmdQueue.Get(), Application::Instance().GetHWND(), &swapChainDesc, nullptr, nullptr,
		(IDXGISwapChain1**)m_swapChain.ReleaseAndGetAddressOf());

	if (FAILED(result)) return false;

	return true;
}

bool Dx12Wrapper::CreateFence()
{
	// �t�F���X�쐬
	auto result = m_device->CreateFence(m_fenceVal, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(m_fence.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	return true;
}

bool Dx12Wrapper::CreateFinalRenderTargets()
{
	// RTV�쐬�i�܂��q�[�v������ė̈���m�ۂ��Ă���RTV���쐬�j
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // �����_�[�^�[�Q�b�g�r���[�Ȃ̂�RTV
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2; // �\����2��
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // ���Ɏw��Ȃ��i�V�F�[�_�[����ǂ������邩�j

	auto result = m_device->CreateDescriptorHeap(&heapDesc,
		IID_PPV_ARGS(m_rtvHeaps.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	DXGI_SWAP_CHAIN_DESC swcDesc{};
	result = m_swapChain->GetDesc(&swcDesc);

	m_backBuffers.resize(swcDesc.BufferCount);

	// �r���[�̃A�h���X
	auto handle = m_rtvHeaps->GetCPUDescriptorHandleForHeapStart();

	// �����_�[�^�[�Q�b�g�r���[�ݒ�
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // �K���}�␳�Ȃ��isRGB�j
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (int i = 0; i < swcDesc.BufferCount; ++i)
	{
		// �X���b�v�`�F�[���쐬���Ɋm�ۂ����o�b�t�@���擾
		result = m_swapChain->GetBuffer(i, IID_PPV_ARGS(m_backBuffers[i].ReleaseAndGetAddressOf()));

		// �擾�����o�b�t�@�̂Ȃ���RTV���쐬����i���Ă������ꂩ�ȁH�j
		m_device->CreateRenderTargetView(m_backBuffers[i].Get(), &rtvDesc, handle);

		// ���̂܂܂��Ɠ������������Ă��܂��̂ŃA�h���X�����炷
		handle.ptr += m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// �r���[�|�[�g�̐ݒ�
	m_viewport.Width = Application::Instance().WindowWidth();   // �o�͐�̕��i�s�N�Z�����j
	m_viewport.Height = Application::Instance().WindowHeight(); // �o�͐�̍����i�s�N�Z�����j
	m_viewport.TopLeftX = 0; // �o�͐�̍�����W X
	m_viewport.TopLeftY = 0; // �o�͐�̍�����W Y
	m_viewport.MaxDepth = 1.0f; // �[�x�ő�l
	m_viewport.MinDepth = 0.0f; // �[�x�ŏ��l

	m_scissorRect.top = 0;                // �؂蔲������W
	m_scissorRect.left = 0;               // �؂蔲�������W
	m_scissorRect.right = Application::Instance().WindowWidth();   // �؂蔲���E���W
	m_scissorRect.bottom = Application::Instance().WindowHeight(); // �؂蔲���������W

	return true;
}

bool Dx12Wrapper::CreateDepthStencilView()
{
	// �[�x�o�b�t�@�[�쐬
	D3D12_RESOURCE_DESC depthResDesc{};
	depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2�����̃e�N�X�`���f�[�^
	depthResDesc.Width = Application::Instance().WindowWidth();   // ���ƍ����̓����_�[�^�[�Q�b�g�Ɠ���
	depthResDesc.Height = Application::Instance().WindowHeight(); // 
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

	auto result = m_device->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, // �[�x�l��������
		&depthClearValue,
		IID_PPV_ARGS(m_depthBuffer.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	// �[�x�̂��߂̃f�B�X�N���v�^�q�[�v�쐬
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{}; // �[�x�Ɏg�����Ƃ��킩��΂悢
	dsvHeapDesc.NumDescriptors = 1; // �[�x�r���[��1��
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV; // �f�v�X�X�e���V���r���[�Ƃ��Ďg��
	result = m_device->CreateDescriptorHeap(&dsvHeapDesc,
		IID_PPV_ARGS(m_dsvHeap.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	// �[�x�r���[�쐬
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // �[�x�l��32�r�b�g�g�p
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2D�e�N�X�`��
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE; // �t���O�͓��ɂȂ�
	m_device->CreateDepthStencilView(m_depthBuffer.Get(), &dsvDesc,
		m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

	return true;
}

bool Dx12Wrapper::CreateSceneView()
{
	// CBV�p�̃q�[�v���쐬
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // �V�F�[�_�[���猩����悤��
	descHeapDesc.NodeMask = 0; // �}�X�N��0
	descHeapDesc.NumDescriptors = 1; // CBV
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	auto result = m_device->CreateDescriptorHeap(&descHeapDesc,
		IID_PPV_ARGS(m_basicDescHeap.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	XMMATRIX worldMat = XMMatrixIdentity();
	XMFLOAT3 eye{ 0.0f, 10.0f, -30.0f };
	XMFLOAT3 target{ 0.0f, 10.0f, 0.0f };
	XMFLOAT3 up{ 0.0f, 1.0f, 0.0f };

	auto viewMat = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
	auto projMat = XMMatrixPerspectiveFovLH(
		XM_PIDIV4,
		static_cast<float>(Application::Instance().WindowWidth())
		/ static_cast<float>(Application::Instance().WindowHeight()),
		1.0f,
		100.0f);

	result = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneMatrix) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_constBuff.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	result = m_constBuff->Map(0, nullptr, (void**)&m_mapMat); // �}�b�v
	//m_mapMat->world = worldMat; // �s��̓��e���R�s�[
	m_mapMat->view = viewMat;
	m_mapMat->proj = projMat;
	m_constBuff->Unmap(0, nullptr);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	cbvDesc.BufferLocation = m_constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = m_constBuff->GetDesc().Width;

	// �萔�o�b�t�@�r���[�쐬
	m_device->CreateConstantBufferView(
		&cbvDesc,
		m_basicDescHeap->GetCPUDescriptorHandleForHeapStart());

	return true;
}

void Dx12Wrapper::CreateTextureLoaderTable()
{
	m_loadLambdaTable["sph"]
		= m_loadLambdaTable["spa"]
		= m_loadLambdaTable["bmp"]
		= m_loadLambdaTable["png"]
		= m_loadLambdaTable["jpg"]
		= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)
		->HRESULT
	{
		return LoadFromWICFile(path.c_str(), 0, meta, img);
	};

	m_loadLambdaTable["tga"]
		= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)
		->HRESULT
	{
		return LoadFromTGAFile(path.c_str(), meta, img);
	};

	m_loadLambdaTable["dds"]
		= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)
		->HRESULT
	{
		return LoadFromDDSFile(path.c_str(), 0, meta, img);
	};
}

bool Dx12Wrapper::CreateWhiteTexture()
{
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4);

	auto result = m_device->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE, // ���Ɏw��Ȃ�
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(m_whiteTex.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		return false;
	}

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0xff); // �S��255�Ŗ��߂�i���j

	result = m_whiteTex->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * 4,
		data.size());

	if (FAILED(result))
	{
		return false;
	}
	
	return true;
}

bool Dx12Wrapper::CreateBlackTexture()
{
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4);

	auto result = m_device->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE, // ���Ɏw��Ȃ�
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(m_blackTex.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		return false;
	}

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0x00); // �S��0�Ŗ��߂�i���j

	result = m_blackTex->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * 4,
		data.size());

	if (FAILED(result))
	{
		return false;
	}

	return true;
}

bool Dx12Wrapper::CreateGrayGradTexture()
{
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, 256);

	auto result = m_device->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE, // ���Ɏw��Ȃ�
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(m_gradTex.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		return false;
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

	result = m_gradTex->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * sizeof(unsigned int),
		sizeof(unsigned int) * data.size());

	if (FAILED(result))
	{
		return false;
	}

	return true;
}

ComPtr<ID3D12Resource> Dx12Wrapper::LoadTextureFromFile(const std::string & texPath)
{
	// WIC�e�N�X�`���̃��[�h
	TexMetadata metadata{};
	ScratchImage scratchImg{};

	auto wtexPath = GetWideStringFromString(texPath); // �e�N�X�`���̃t�@�C���p�X
	auto ext = GetExtension(texPath);

	if (ext == "")
	{
		return nullptr;
	}

	auto result = m_loadLambdaTable[ext](
		wtexPath,
		&metadata,
		scratchImg);

	if (FAILED(result))
	{
		return nullptr;
	}

	auto img = scratchImg.GetImage(0, 0, 0); // ���f�[�^���o

	// WriteToSubresource�œ]������p�̃q�[�v�ݒ�
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
	ComPtr<ID3D12Resource> texBuff{ nullptr };
	result = m_device->CreateCommittedResource(
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

	m_resourceTable[texPath] = texBuff;

	return texBuff;
}
