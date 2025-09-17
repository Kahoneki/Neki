#pragma once
#include "ICommandPool.h"
#include "glm/vec2.hpp"
#include "Types/ResourceStates.h"

namespace NK
{
	enum class COMMAND_BUFFER_LEVEL
	{
		PRIMARY,
		SECONDARY,
	};

	struct CommandBufferDesc
	{
		COMMAND_BUFFER_LEVEL level;
	};

	enum class PIPELINE_BIND_POINT
	{
		GRAPHICS,
		COMPUTE,
	};


	class ICommandBuffer
	{
	public:
		virtual ~ICommandBuffer() = default;

		//todo: add command buffer methods here
		virtual void Reset() = 0;

		virtual void Begin() = 0;
		virtual void SetBlendConstants(const float _blendConstants[4]) = 0;
		virtual void End() = 0;

		virtual void TransitionBarrier(ITexture* _texture, RESOURCE_STATE _oldState, RESOURCE_STATE _newState) = 0;
		virtual void BeginRendering(std::size_t _numColourAttachments, ITextureView* _colourAttachments, ITextureView* _depthStencilAttachment) = 0;
		virtual void EndRendering() = 0;

		virtual void BindPipeline(IPipeline* _pipeline, PIPELINE_BIND_POINT _bindPoint) = 0;
		virtual void SetViewport(glm::vec2 _pos, glm::vec2 _extent) = 0;
		virtual void SetScissor(glm::ivec2 _pos, glm::ivec2 _extent) = 0;
		virtual void Draw(std::uint32_t _vertexCount, std::uint32_t _instanceCount, std::uint32_t _firstVertex, std::uint32_t _firstInstance) = 0;


	protected:
		explicit ICommandBuffer(ILogger& _logger, IDevice& _device, ICommandPool& _pool, const CommandBufferDesc& _desc)
		: m_logger(_logger), m_device(_device), m_pool(_pool), m_level(_desc.level) {}



		//Dependency injections
		ILogger& m_logger;
		IDevice& m_device;
		ICommandPool& m_pool;

		COMMAND_BUFFER_LEVEL m_level;
	};
}