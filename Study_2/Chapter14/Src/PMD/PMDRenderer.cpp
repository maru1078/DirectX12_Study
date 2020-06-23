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
	// パイプラインをセット
	m_dx12.lock()->CommandList()->SetPipelineState(m_plsShadow.Get());

	// ルートシグネチャをセット
	m_dx12.lock()->CommandList()->SetGraphicsRootSignature(m_rootSignature.Get());
}

void PMDRenderer::PreDrawPMD()
{
	// パイプラインをセット
	m_dx12.lock()->CommandList()->SetPipelineState(m_pipelineState.Get());

	// ルートシグネチャをセット
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
	// レンジの作成
	CD3DX12_DESCRIPTOR_RANGE range[5]{};
	range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // ビュープロジェクションなどの変換行列
	range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // ワールド変換行列（モデルそれぞれ）
	range[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2); // マテリアル用CBV
	range[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0); // マテリアル用SRV
	range[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4); // ライトデプス用SRV

	// ルートパラメーター作成
	CD3DX12_ROOT_PARAMETER rootParam[4]{};
	rootParam[0].InitAsDescriptorTable(1, &range[0]);
	rootParam[1].InitAsDescriptorTable(1, &range[1]);
	rootParam[2].InitAsDescriptorTable(2, &range[2]);
	rootParam[3].InitAsDescriptorTable(1, &range[4]); // ライトデプス用

	// サンプラー作成
	CD3DX12_STATIC_SAMPLER_DESC samplerDesc[3]{};
	samplerDesc[0].Init(0);
	samplerDesc[1].Init(1, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	
	// ほとんど同じなので通常のサンプラー情報をコピー
	samplerDesc[2] = samplerDesc[0];
	samplerDesc[2].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[2].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[2].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

	// <=であればtrue(1.0)、そうでなければfalse(0.0)
	samplerDesc[2].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	samplerDesc[2].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR; // 比較結果をバイリニア補間
	samplerDesc[2].MaxAnisotropy = 1; // 深度傾斜を有効に
	samplerDesc[2].ShaderRegister = 2; // 2番スロット

	// ルートシグネチャ作成
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.pParameters = rootParam; // ルートパラメーターの先頭アドレス
	rootSignatureDesc.NumParameters = _countof(rootParam); // ルートパラメーター数
	rootSignatureDesc.pStaticSamplers = samplerDesc;
	rootSignatureDesc.NumStaticSamplers = _countof(samplerDesc);

	ID3DBlob* rootSigBlob{ nullptr };
	auto result = D3D12SerializeRootSignature(
		&rootSignatureDesc, // ルートシグネチャ設定
		D3D_ROOT_SIGNATURE_VERSION_1_0, // ルートシグネチャバージョン
		&rootSigBlob, // シェーダーを作った時と同じ
		m_errorBlob.ReleaseAndGetAddressOf());

	if (FAILED(result)) return false;

	result = m_dx12.lock()->Device()->CreateRootSignature(
		0, // nodemask。0でよい
		rootSigBlob->GetBufferPointer(), // シェーダーのときと同じ
		rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	rootSigBlob->Release(); // 不要になったので解放

	return true;
}

bool PMDRenderer::CreatePipeline()
{
	// 頂点シェーダー
	auto result = D3DCompileFromFile(
		L"Shader/BasicVertexShader.hlsl", // シェーダー名
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルードはデフォルト
		"BasicVS", // 関数は「BasicVS」
		"vs_5_0", // 対象シェーダーは「vs_5_0」
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用及び最適化はなし
		0,
		m_vsBlob.ReleaseAndGetAddressOf(),
		m_errorBlob.ReleaseAndGetAddressOf()); // エラー時はメッセージが入る

	if (FAILED(result)) return false;

	// ピクセルシェーダー
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
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // 座標
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // 法線
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // uv
		{ "BONE_NO",  0, DXGI_FORMAT_R16G16_UINT,     0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // ボーン番号
		{ "WEIGHT",   0, DXGI_FORMAT_R8_UINT,         0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // ウェイト
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline{};

	gpipeline.pRootSignature = m_rootSignature.Get();

	// 頂点シェーダー設定
	gpipeline.VS = CD3DX12_SHADER_BYTECODE(m_vsBlob.Get());

	// ピクセルシェーダー設定
	gpipeline.PS = CD3DX12_SHADER_BYTECODE(m_psBlob.Get());

	// デフォルトのサンプルマスクを表す定数（0xffffffff）
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	// ラスタライザー設定
	gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // カリングしない

	// ブレンドステート設定
	gpipeline.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	gpipeline.InputLayout.pInputElementDescs = inputLayout; // レイアウト先頭アドレス
	gpipeline.InputLayout.NumElements = _countof(inputLayout); // レイアウト配列の要素数

	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED; // カットなし

	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // 三角形で構成

	// レンダーターゲット設定
	gpipeline.NumRenderTargets = 3;
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // 0～1に正規化されたRGBA
	gpipeline.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gpipeline.RTVFormats[2] = DXGI_FORMAT_R8G8B8A8_UNORM;

	// サンプル数指定
	gpipeline.SampleDesc.Count = 1; // サンプリングは 1 ピクセルにつき 1
	gpipeline.SampleDesc.Quality = 0; // クオリティは最低

	// 深度バッファー周りの設定
	gpipeline.DepthStencilState.DepthEnable = true; // 深度バッファを使う
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // ピクセル描画時に深度情報をに書き込む
	gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // 小さい方を採用
	gpipeline.DepthStencilState.StencilEnable = false;
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	// パイプライン作成
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
