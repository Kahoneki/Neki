#pragma once
#include <RHI/ISurface.h>
#include "D3D12Device.h"

namespace NK
{

	class D3D12Surface final : public ISurface
	{
	public:
		explicit D3D12Surface(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const SurfaceDesc& _desc);
		virtual ~D3D12Surface() override;

		//D3D12 internal API (for use by other RHI-D3D12 classes)
		[[nodiscard]] inline HWND GetSurface() const { return m_surface; }

	private:
		void CreateGLFWWindow(); //"CreateWindow" is a win32 macro, great
		void CreateSurface();

		HWND m_surface;
	};

}