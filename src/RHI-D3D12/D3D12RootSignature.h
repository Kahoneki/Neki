#pragma once
#include <RHI/IRootSignature.h>
#include "D3D12Device.h"

namespace NK
{
	class D3D12RootSignature final : public IRootSignature
	{
	public:
		explicit D3D12RootSignature(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const RootSignatureDesc& _desc);
		virtual ~D3D12RootSignature() override;

		//D3D12 internal API (for use by other RHI-D3D12 classes)
		struct RootDescriptorTable
		{
			std::size_t rootParamIndex;
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
		};
		[[nodiscard]] inline ID3D12RootSignature* GetRootSignature() const { return m_rootSig.Get(); }
		[[nodiscard]] inline ID3D12DescriptorHeap* GetResourceDescriptorHeap() const { return m_resourceDescriptorHeap; }
		[[nodiscard]] inline ID3D12DescriptorHeap* GetSamplerDescriptorHeap() const { return m_samplerDescriptorHeap; }
		[[nodiscard]] inline RootDescriptorTable GetResourceDescriptorTable() const { return m_resourceDescriptorTable; }
		[[nodiscard]] inline RootDescriptorTable GetSamplerDescriptorTable() const { return m_samplerDescriptorTable; }
		

	private:
		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSig;
		ID3D12DescriptorHeap* m_resourceDescriptorHeap; //Owned by D3D12Device
		ID3D12DescriptorHeap* m_samplerDescriptorHeap; //Owned by D3D12Device
		RootDescriptorTable m_resourceDescriptorTable;
		RootDescriptorTable m_samplerDescriptorTable;
	};
}