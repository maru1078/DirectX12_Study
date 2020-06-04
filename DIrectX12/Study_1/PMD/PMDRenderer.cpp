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
		L"BasicVertexShader.hlsl", // シェーダー名
		nullptr, // defineはなし
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルードはデフォルト
		"BasicVS", "vs_5_0", // 関数は BasicVS、対象シェーダーは vs_5_0
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用及び最適化なし
		0,
		&vsBlob, &errorBlob // エラー時は errorBlob にメッセージが入る
	);

	if (!CheckShaderCompileResult(result, errorBlob.Get()))
	{
		assert(0);
		return result;
	}

	result = D3DCompileFromFile(
		L"BasicPixelShader.hlsl", // シェーダー名
		nullptr, // defineはなし
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルードはデフォルト
		"BasicPS", "ps_5_0", // 関数は BasicPS、対象シェーダーは ps_5_0
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用及び最適化なし
		0,
		&psBlob, &errorBlob // エラー時は errorBlob にメッセージが入る
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
		{ // uv（追加）
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

	// デフォルトのサンプルマスクを表す定数（0xffffffff）
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // カリングしない

	gpipeline.DepthStencilState.DepthEnable = true; // 深度バッファーを使う
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // 書き込む
	gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // 小さいほうを採用
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	gpipeline.DepthStencilState.StencilEnable = false;

	gpipeline.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	gpipeline.InputLayout.pInputElementDescs = inputLayout;    // レイアウト先頭アドレス
	gpipeline.InputLayout.NumElements = _countof(inputLayout); // レイアウト配列の要素数

	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED; // カットなし

	// 三角形で構成
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	gpipeline.NumRenderTargets = 1; // 今は1つのみ
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // 0 ～ 1 に正規化されたRGBA

	gpipeline.SampleDesc.Count = 1;   // サンプリングは 1 ピクセルにつき 1
	gpipeline.SampleDesc.Quality = 0; // クオリティは最低

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
	descTblRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // 定数[b0]（座標変換用）
	descTblRange[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // 定数[b1]（ワールド）
	descTblRange[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2); // 定数[b2]（マテリアル用）
	descTblRange[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0); // テクスチャ4つ

	CD3DX12_ROOT_PARAMETER rootParam[3] = {};
	rootParam[0].InitAsDescriptorTable(1, &descTblRange[0]); // 座標変換
	rootParam[1].InitAsDescriptorTable(1, &descTblRange[1]); // ワールド
	rootParam[2].InitAsDescriptorTable(2, &descTblRange[2]); // マテリアル周り

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
		&rootSignatureDesc,             // ルートシグネチャ設定
		D3D_ROOT_SIGNATURE_VERSION_1_0, // ルートシグネチャバージョン
		&rootSigBlob,                   // シェーダーを作った時と同じ
		&errorBlob                      // エラー処理も同じ
	);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	result = m_dx12->Device()->CreateRootSignature(
		0,                               // nodemask。0でよい
		rootSigBlob->GetBufferPointer(), // シェーダーのときと同様
		rootSigBlob->GetBufferSize(),    // シェーダーのときと同様
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
	// 頂点シェーダー作成
	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA("ファイルが見当たりません");
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
