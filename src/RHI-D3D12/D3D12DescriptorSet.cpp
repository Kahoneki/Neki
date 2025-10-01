#include "D3D12DescriptorSet.h"

namespace NK
{

	D3D12DescriptorSet::D3D12DescriptorSet(Microsoft::WRL::ComPtr<ID3D12RootSignature> _rootSig)
		: m_rootSig(std::move(_rootSig))
	{
	}



	D3D12DescriptorSet::~D3D12DescriptorSet()
	{
		//ComPtrs are released automatically
	}

}