#include "PMDRenderer.h"

#include "../Dx12Wrapper/Dx12Wrapper.h"
#include "PMDActor.h"

#pragma comment(lib, "d3dcompiler.lib")

PMDRenderer::PMDRenderer(std::weak_ptr<Dx12Wrapper> dx12)
	: m_dx12{ dx12 }
{
	if (!CreateRootSignature())
	{
		assert(0);
	}
	if (!CreatePipeline())
	{
		assert(0);
	}
}

PMDRenderer::~PMDRenderer()
{
}

void PMDRenderer::Update()
{
	for (auto actor : m_actors)
	{
		actor->Update();
	}
}

void PMDRenderer::PreDrawFromLight()
{
	// �p�C�v���C�����Z�b�g
	m_dx12.lock()->CommandList()->SetPipelineState(m_plsShadow.Get());

	// ���[�g�V�O�l�`�����Z�b�g
	m_dx12.lock()->CommandList()->SetGraphicsRootSignature(m_rootSignature.Get());
}

void PMDRenderer::PreDrawPMD()
{
	// �p�C�v���C�����Z�b�g
	m_dx12.lock()->CommandList()->SetPipelineState(m_pipelineState.Get());

	// ���[�g�V�O�l�`�����Z�b�g
	m_dx12.lock()->CommandList()->SetGraphicsRootSignature(m_rootSignature.Get());
}

void PMDRenderer::DrawFromLight()
{
	for (auto actor : m_actors)
	{
		actor->Draw(true);
	}
}

void PMDRenderer::DrawPMD()
{
	for (auto actor : m_actors)
	{
		actor->Draw();
	}
}

void PMDRenderer::AddActor(std::shared_ptr<PMDActor> actor)
{
	m_actors.push_back(actor);
}

bool PMDRenderer::CreateRootSignature()
{
	// �����W�̍쐬
	CD3DX12_DESCRIPTOR_RANGE range[5]{};
	range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // �r���[�v���W�F�N�V�����Ȃǂ̕ϊ��s��
	range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // ���[���h�ϊ��s��i���f�����ꂼ��j
	range[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2); // �}�e���A���pCBV
	range[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0); // �}�e���A���pSRV
	range[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4); // ���C�g�f�v�X�pSRV

	// ���[�g�p�����[�^�[�쐬
	CD3DX12_ROOT_PARAMETER rootParam[4]{};
	rootParam[0].InitAsDescriptorTable(1, &range[0]);
	rootParam[1].InitAsDescriptorTable(1, &range[1]);
	rootParam[2].InitAsDescriptorTable(2, &range[2]);
	rootParam[3].InitAsDescriptorTable(1, &range[4]); // ���C�g�f�v�X�p

	// �T���v���[�쐬
	CD3DX12_STATIC_SAMPLER_DESC samplerDesc[3]{};
	samplerDesc[0].Init(0);
	samplerDesc[1].Init(1, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	
	// �قƂ�Ǔ����Ȃ̂Œʏ�̃T���v���[�����R�s�[
	samplerDesc[2] = samplerDesc[0];
	samplerDesc[2].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[2].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[2].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

	// <=�ł����true(1.0)�A�����łȂ����false(0.0)
	samplerDesc[2].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	samplerDesc[2].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR; // ��r���ʂ��o�C���j�A���
	samplerDesc[2].MaxAnisotropy = 1; // �[�x�X�΂�L����
	samplerDesc[2].ShaderRegister = 2; // 2�ԃX���b�g

	// ���[�g�V�O�l�`���쐬
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.pParameters = rootParam; // ���[�g�p�����[�^�[�̐擪�A�h���X
	rootSignatureDesc.NumParameters = _countof(rootParam); // ���[�g�p�����[�^�[��
	rootSignatureDesc.pStaticSamplers = samplerDesc;
	rootSignatureDesc.NumStaticSamplers = _countof(samplerDesc);

	ID3DBlob* rootSigBlob{ nullptr };
	auto result = D3D12SerializeRootSignature(
		&rootSignatureDesc, // ���[�g�V�O�l�`���ݒ�
		D3D_ROOT_SIGNATURE_VERSION_1_0, // ���[�g�V�O�l�`���o�[�W����
		&rootSigBlob, // �V�F�[�_�[����������Ɠ���
		m_errorBlob.ReleaseAndGetAddressOf());

	if (FAILED(result)) return false;

	result = m_dx12.lock()->Device()->CreateRootSignature(
		0, // nodemask�B0�ł悢
		rootSigBlob->GetBufferPointer(), // �V�F�[�_�[�̂Ƃ��Ɠ���
		rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	rootSigBlob->Release(); // �s�v�ɂȂ����̂ŉ��

	return true;
}

bool PMDRenderer::CreatePipeline()
{
	// ���_�V�F�[�_�[
	auto result = D3DCompileFromFile(
		L"Shader/BasicVertexShader.hlsl", // �V�F�[�_�[��
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // �C���N���[�h�̓f�t�H���g
		"BasicVS", // �֐��́uBasicVS�v
		"vs_5_0", // �ΏۃV�F�[�_�[�́uvs_5_0�v
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // �f�o�b�O�p�y�эœK���͂Ȃ�
		0,
		m_vsBlob.ReleaseAndGetAddressOf(),
		m_errorBlob.ReleaseAndGetAddressOf()); // �G���[���̓��b�Z�[�W������

	if (FAILED(result)) return false;

	// �s�N�Z���V�F�[�_�[
	result = D3DCompileFromFile(
		L"Shader/BasicPixelShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicPS",
		"ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		m_psBlob.ReleaseAndGetAddressOf(),
		m_errorBlob.ReleaseAndGetAddressOf());

	if (FAILED(result)) return false;

	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // ���W
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // �@��
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // uv
		{ "BONE_NO",  0, DXGI_FORMAT_R16G16_UINT,     0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // �{�[���ԍ�
		{ "WEIGHT",   0, DXGI_FORMAT_R8_UINT,         0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // �E�F�C�g
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline{};

	gpipeline.pRootSignature = m_rootSignature.Get();

	// ���_�V�F�[�_�[�ݒ�
	gpipeline.VS = CD3DX12_SHADER_BYTECODE(m_vsBlob.Get());

	// �s�N�Z���V�F�[�_�[�ݒ�
	gpipeline.PS = CD3DX12_SHADER_BYTECODE(m_psBlob.Get());

	// �f�t�H���g�̃T���v���}�X�N��\���萔�i0xffffffff�j
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	// ���X�^���C�U�[�ݒ�
	gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // �J�����O���Ȃ�

	// �u�����h�X�e�[�g�ݒ�
	gpipeline.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	gpipeline.InputLayout.pInputElementDescs = inputLayout; // ���C�A�E�g�擪�A�h���X
	gpipeline.InputLayout.NumElements = _countof(inputLayout); // ���C�A�E�g�z��̗v�f��

	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED; // �J�b�g�Ȃ�

	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // �O�p�`�ō\��

	// �����_�[�^�[�Q�b�g�ݒ�
	gpipeline.NumRenderTargets = 3;
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // 0�`1�ɐ��K�����ꂽRGBA
	gpipeline.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gpipeline.RTVFormats[2] = DXGI_FORMAT_R8G8B8A8_UNORM;

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
	result = m_dx12.lock()->Device()->CreateGraphicsPipelineState(
		&gpipeline,
		IID_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	result = D3DCompileFromFile(
		L"Shader/BasicVertexShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"ShadowVS",
		"vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		m_vsBlob.ReleaseAndGetAddressOf(),
		m_errorBlob.ReleaseAndGetAddressOf());

	if (FAILED(result)) return false;

	gpipeline.VS = CD3DX12_SHADER_BYTECODE(m_vsBlob.Get());
	gpipeline.PS.BytecodeLength = 0;
	gpipeline.PS.pShaderBytecode = 0;
	//gpipeline.NumRenderTargets = 0;
	gpipeline.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	//gpipeline.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;

	result = m_dx12.lock()->Device()->CreateGraphicsPipelineState(
		&gpipeline,
		IID_PPV_ARGS(m_plsShadow.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	return true;
}
