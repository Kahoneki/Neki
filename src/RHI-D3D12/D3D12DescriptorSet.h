#pragma once
#include <RHI/IDescriptorSet.h>
#include "D3D12Device.h"

namespace NK
{
	class D3D12DescriptorSet final : public IDescriptorSet
	{
	public:
		explicit D3D12DescriptorSet(Microsoft::WRL::ComPtr<ID3D12RootSignature> _rootSig);
		virtual ~D3D12DescriptorSet() override;

		//D3D12 internal API (for use by other RHI-D3D12 classes)
		[[nodiscard]] inline ID3D12RootSignature* GetRootSignature() const { return m_rootSig.Get(); }


	private:
		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSig;
	};
}