#pragma once
#include <RHI/IDevice.h>
#include <RHI/ICommandPool.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>



namespace NK
{
	class D3D12Device final : public IDevice
	{
	public:
		explicit D3D12Device(ILogger& _logger, IAllocator& _allocator);
		virtual ~D3D12Device() override;

		//IDevice interface implementation
		//[[nodiscard]] virtual UniquePtr<IBuffer> CreateBuffer(const BufferDesc& _desc) override;
		//[[nodiscard]] virtual UniquePtr<ITexture> CreateTexture(const TextureDesc& _desc) override;
		[[nodiscard]] virtual UniquePtr<ICommandPool> CreateCommandPool(const CommandPoolDesc& _desc) override;
		//[[nodiscard]] virtual UniquePtr<ISurface> CreateSurface(const Window* _window) override;

		//D3D12 internal API (for use by other RHI-D3D12 classes)
		[[nodiscard]] Microsoft::WRL::ComPtr<IDXGIFactory> GetFactory() const { return m_factory; }
		[[nodiscard]] Microsoft::WRL::ComPtr<IDXGIAdapter> GetAdapter() const { return m_adapter; }
		[[nodiscard]] Microsoft::WRL::ComPtr<ID3D12Device> GetDevice() const { return m_device; }
		[[nodiscard]] Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetGraphicsQueue() const { return m_graphicsQueue; }
		[[nodiscard]] Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetComputeQueue() const { return m_computeQueue; }
		[[nodiscard]] Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetTransferQueue() const { return m_transferQueue; }


	private:
		//Init sub-methods
		void EnableDebugLayer();
		void CreateFactory();
		void SelectAdapter();
		void CreateDevice();
		void CreateCommandQueues();


		const D3D_FEATURE_LEVEL m_featureLevel{ D3D_FEATURE_LEVEL_12_0 };

		//D3D12 handles
		Microsoft::WRL::ComPtr<IDXGIFactory4> m_factory;
		Microsoft::WRL::ComPtr<IDXGIAdapter1> m_adapter;
		Microsoft::WRL::ComPtr<ID3D12Device> m_device;

		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_graphicsQueue;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_computeQueue;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_transferQueue;


		bool m_enableDebugLayer{ true };
	};
}