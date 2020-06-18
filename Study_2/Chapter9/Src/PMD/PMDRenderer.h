#ifndef PMD_RENDERER_H_
#define PMD_RENDERER_H_

#include <memory>
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

	void AddActor(PMDActor* actor);

	void PreDrawPMD();
	void Draw();

	bool CreateRootSignature();
	bool CreatePipeline();

private:

	std::weak_ptr<Dx12Wrapper> m_dx12;
	std::vector<PMDActor*> m_pmdActors;

	ComPtr<ID3DBlob> vsBlob{ nullptr };
	ComPtr<ID3DBlob> psBlob{ nullptr };
	ComPtr<ID3DBlob> errorBlob{ nullptr };
	ComPtr<ID3D12RootSignature> rootSignature{ nullptr };
	ComPtr<ID3D12PipelineState> _pipelineState{ nullptr };
};

#endif // !PMD_RENDERER_H_
