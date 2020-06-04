#include "PMDRenderer.h"
#include <d3dx12.h>
#include <d3dcompiler.h>
#include "PMDActor.h"

#include "../Dx12Wrapper/Dx12Wrapper.h"

HRESULT PMDRenderer::CreateGraphicsPipelineForPMD()
{
	ComPtr<ID3DBlob> vsBlob{ nullptr };
	ComPtr<ID3DBlob> psBlob{ nullptr };
	ComPtr<ID3DBlob> errorBlob{ nullptr };

	auto result = D3DCompileFromFile(
		L"BasicVertexShader.hlsl", // �V�F�[�_�[��
		nullptr, // define�͂Ȃ�
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // �C���N���[�h�̓f�t�H���g
		"BasicVS", "vs_5_0", // �֐��� BasicVS�A�ΏۃV�F�[�_�[�� vs_5_0
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // �f�o�b�O�p�y�эœK���Ȃ�
		0,
		&vsBlob, &errorBlob // �G���[���� errorBlob �Ƀ��b�Z�[�W������
	);

	if (!CheckShaderCompileResult(result, errorBlob.Get()))
	{
		assert(0);
		return result;
	}

	result = D3DCompileFromFile(
		L"BasicPixelShader.hlsl", // �V�F�[�_�[��
		nullptr, // define�͂Ȃ�
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // �C���N���[�h�̓f�t�H���g
		"BasicPS", "ps_5_0", // �֐��� BasicPS�A�ΏۃV�F�[�_�[�� ps_5_0
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // �f�o�b�O�p�y�эœK���Ȃ�
		0,
		&psBlob, &errorBlob // �G���[���� errorBlob �Ƀ��b�Z�[�W������
	);

	if (!CheckShaderCompileResult(result, errorBlob.Get()))
	{
		assert(0);
		return result;
	}

	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{
			"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
		{
			"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA
		},
		{ // uv�i�ǉ��j
			"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
		{
			"BONENO", 0, DXGI_FORMAT_R16G16_UINT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
		{
			"WEIGHT", 0, DXGI_FORMAT_R8_UINT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
		/*	{
				"EDGE_FLG", 0, DXGI_FORMAT_R8_UINT, 0,
				D3D12_APPEND_ALIGNED_ELEMENT,
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
			}*/

	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline{};
	gpipeline.pRootSignature = m_rootSignature.Get();
	gpipeline.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
	gpipeline.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());

	// �f�t�H���g�̃T���v���}�X�N��\���萔�i0xffffffff�j
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // �J�����O���Ȃ�

	gpipeline.DepthStencilState.DepthEnable = true; // �[�x�o�b�t�@�[���g��
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // ��������
	gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // �������ق����̗p
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	gpipeline.DepthStencilState.StencilEnable = false;

	gpipeline.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	gpipeline.InputLayout.pInputElementDescs = inputLayout;    // ���C�A�E�g�擪�A�h���X
	gpipeline.InputLayout.NumElements = _countof(inputLayout); // ���C�A�E�g�z��̗v�f��

	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED; // �J�b�g�Ȃ�

	// �O�p�`�ō\��
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	gpipeline.NumRenderTargets = 1; // ����1�̂�
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // 0 �` 1 �ɐ��K�����ꂽRGBA

	gpipeline.SampleDesc.Count = 1;   // �T���v�����O�� 1 �s�N�Z���ɂ� 1
	gpipeline.SampleDesc.Quality = 0; // �N�I���e�B�͍Œ�

	result = m_dx12->Device()->CreateGraphicsPipelineState(
		&gpipeline, IID_PPV_ARGS(m_pipeline.ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	return result;
}

HRESULT PMDRenderer::CreateRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE descTblRange[4]{};
	descTblRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // �萔[b0]�i���W�ϊ��p�j
	descTblRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // �萔[b1]�i���[���h�j
	descTblRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2); // �萔[b2]�i�}�e���A���p�j
	descTblRange[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0); // �e�N�X�`��4��

	CD3DX12_ROOT_PARAMETER rootParam[3] = {};
	rootParam[0].InitAsDescriptorTable(1, &descTblRange[0]); // ���W�ϊ�
	rootParam[1].InitAsDescriptorTable(1, &descTblRange[1]); // ���[���h
	rootParam[2].InitAsDescriptorTable(2, &descTblRange[2]); // �}�e���A������

	CD3DX12_STATIC_SAMPLER_DESC samplerDesc[2]{};
	samplerDesc[0].Init(0);
	samplerDesc[1].Init(
		1,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Init(
		3,
		rootParam,
		2,
		samplerDesc,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> rootSigBlob{ nullptr };
	ComPtr<ID3DBlob> errorBlob{ nullptr };

	auto result = D3D12SerializeRootSignature(
		&rootSignatureDesc,             // ���[�g�V�O�l�`���ݒ�
		D3D_ROOT_SIGNATURE_VERSION_1_0, // ���[�g�V�O�l�`���o�[�W����
		&rootSigBlob,                   // �V�F�[�_�[����������Ɠ���
		&errorBlob                      // �G���[����������
	);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	result = m_dx12->Device()->CreateRootSignature(
		0,                               // nodemask�B0�ł悢
		rootSigBlob->GetBufferPointer(), // �V�F�[�_�[�̂Ƃ��Ɠ��l
		rootSigBlob->GetBufferSize(),    // �V�F�[�_�[�̂Ƃ��Ɠ��l
		IID_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	return result;
}

bool PMDRenderer::CheckShaderCompileResult(HRESULT result, ID3DBlob * error)
{
	// ���_�V�F�[�_�[�쐬
	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA("�t�@�C������������܂���");
			return false;
		}
		else
		{
			std::string errstr;
			errstr.resize(error->GetBufferSize());

			std::copy_n((char*)error->GetBufferPointer(),
				error->GetBufferSize(),
				errstr.begin());

			errstr += "\n";

			::OutputDebugStringA(errstr.c_str());
		}

		return false;
	}

	return true;
}

PMDRenderer::PMDRenderer(std::shared_ptr<Dx12Wrapper> dx12)
	: m_dx12(dx12)
{
	assert(SUCCEEDED(CreateRootSignature()));
	assert(SUCCEEDED(CreateGraphicsPipelineForPMD()));
}

PMDRenderer::~PMDRenderer()
{
}

void PMDRenderer::AddActor(std::shared_ptr<PMDActor> actor)
{
	m_actors.push_back(actor);
}

void PMDRenderer::Update()
{
	for (auto& actor : m_actors)
	{
		actor->Update();
	}
}

void PMDRenderer::BeforeDraw()
{
	m_dx12->CommandList()->SetPipelineState(m_pipeline.Get());
	m_dx12->CommandList()->SetGraphicsRootSignature(m_rootSignature.Get());
}

void PMDRenderer::Draw()
{
	for (auto& actor : m_actors)
	{
		actor->Draw();
	}
}

ID3D12PipelineState * PMDRenderer::GetPipelineState()
{
	return m_pipeline.Get();
}

ID3D12RootSignature * PMDRenderer::GetRootSignature()
{
	return m_rootSignature.Get();
}
