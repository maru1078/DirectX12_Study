#ifndef PMD_RENDERER_H_
#define PMD_RENDERER_H_

#include <memory>
#include <vector>
#include <d3d12.h>
#include <d3dx12.h>
#include <d3dcompiler.h>
#include <wrl.h>

class Dx12Wrapper;
class PMDActor;

using namespace Microsoft::WRL;

class PMDRenderer
{
public:

	PMDRenderer(std::weak_ptr<Dx12Wrapper> dx12);
	~PMDRenderer();

	void Update();

	void PreDrawFromLight();
	void PreDrawPMD();

	void DrawFromLight();
	void DrawPMD();

	void AddActor(std::shared_ptr<PMDActor> actor);

private:

	bool CreateRootSignature();
	bool CreatePipeline();

private:

	std::weak_ptr<Dx12Wrapper> m_dx12;

	std::vector<std::shared_ptr<PMDActor>> m_actors;
	ComPtr<ID3DBlob> m_vsBlob{ nullptr };
	ComPtr<ID3DBlob> m_psBlob{ nullptr };
	ComPtr<ID3DBlob> m_errorBlob{ nullptr };
	ComPtr<ID3D12RootSignature> m_rootSignature{ nullptr };
	ComPtr<ID3D12PipelineState> m_pipelineState{ nullptr };
	ComPtr<ID3D12PipelineState> m_plsShadow{ nullptr }; // ライトデプス用
};

#endif // !PMD_RENDERER_H_
