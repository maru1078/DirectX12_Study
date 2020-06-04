#pragma once

#include<d3d12.h>
#include<vector>
#include<wrl.h>
#include<memory>

using namespace Microsoft::WRL;

class Dx12Wrapper;
class PMDActor;

class PMDRenderer
{
	friend PMDActor;

	std::shared_ptr<Dx12Wrapper> m_dx12;
	std::vector<std::shared_ptr<PMDActor>> m_actors;

	ComPtr<ID3D12PipelineState> m_pipeline{ nullptr }; // PMD�p�p�C�v���C��
	ComPtr<ID3D12RootSignature> m_rootSignature{ nullptr };

	ComPtr<ID3D12PipelineState> m_plsShadow; // �e�p�̃p�C�v���C��

private:

	// �p�C�v���C��������
	HRESULT CreateGraphicsPipelineForPMD();

	// ���[�g�V�O�l�`��������
	HRESULT CreateRootSignature();

	bool CheckShaderCompileResult(HRESULT result, ID3DBlob* error = nullptr);

public:

	PMDRenderer(std::shared_ptr<Dx12Wrapper> dx12);
	~PMDRenderer();

	void AddActor(std::shared_ptr<PMDActor> actor);

	void Update();
	void BeforeDrawFromLight();
	void DrawFromLight();
	void BeforeDraw();
	void Draw();
	ID3D12PipelineState* GetPipelineState();
	ID3D12RootSignature* GetRootSignature();

};