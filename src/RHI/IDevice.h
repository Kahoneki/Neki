#pragma once
#include "IDevice.h"

namespace NK
{
	class IBuffer;
	struct BufferDesc;

	class ITexture;
	struct TextureDesc;

	class ICommandQueue;
	struct CommandQueueDesc;

	class Window;
	class ISurface;
}



namespace NK
{

	class IDevice
	{
	public:
		virtual ~IDevice() = default;
		
		[[nodiscard]] virtual IBuffer* CreateBuffer(const BufferDesc& _desc) = 0;
		[[nodiscard]] virtual ITexture* CreateTexture(const TextureDesc& _desc) = 0;
		[[nodiscard]] virtual ICommandQueue* CreateCommandQueue(const CommandQueueDesc& _desc) = 0;
		[[nodiscard]] virtual ISurface* CreateSurface(const Window* _window) = 0;
	};

}