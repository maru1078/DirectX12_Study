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

	void PreDrawPMD();

	bool CreateRootSignature();
	bool CreatePipeline();

private:

	std::weak_ptr<Dx12Wrapper> m_dx12;

	ComPtr<ID3DBlob> m_vsBlob{ nullptr };
	ComPtr<ID3DBlob> m_psBlob{ nullptr };
	ComPtr<ID3DBlob> m_errorBlob{ nullptr };
	ComPtr<ID3D12RootSignature> m_rootSignature{ nullptr };
	ComPtr<ID3D12PipelineState> m_pipelineState{ nullptr };
};

#endif // !PMD_RENDERER_H_
