#pragma once
#include "RHI/ICommandBuffer.h"

namespace NK
{
	struct BarrierInfo
	{
		VkImageLayout layout;
		VkAccessFlags accessMask;
		VkPipelineStageFlags stageMask;
	};
	
	
	class VulkanCommandBuffer final : public ICommandBuffer
	{
	public:
		explicit VulkanCommandBuffer(ILogger& _logger, IDevice& _device, ICommandPool& _pool, const CommandBufferDesc& _desc);
		virtual ~VulkanCommandBuffer() override;

		//todo: add command buffer methods here
		virtual void Reset() override;

		virtual void Begin() override;
		virtual void SetBlendConstants(const float _blendConstants[4]) override;
		virtual void End() override;

		virtual void TransitionBarrier(ITexture* _texture, RESOURCE_STATE _oldState, RESOURCE_STATE _newState) override;
		virtual void BeginRendering(std::size_t _numColourAttachments, ITextureView* _colourAttachments, ITextureView* _depthAttachment, ITextureView* _stencilAttachment) override;
		virtual void EndRendering() override;

		virtual void BindPipeline(IPipeline* _pipeline, PIPELINE_BIND_POINT _bindPoint) override;
		virtual void SetViewport(glm::vec2 _pos, glm::vec2 _extent) override;
		virtual void SetScissor(glm::ivec2 _pos, glm::ivec2 _extent) override;
		virtual void Draw(std::uint32_t _vertexCount, std::uint32_t _instanceCount, std::uint32_t _firstVertex, std::uint32_t _firstInstance) override;
		
		//Vulkan internal API (for use by other RHI-Vulkan classes)
		[[nodiscard]] inline VkCommandBuffer GetBuffer() const { return m_buffer; }


	private:
		BarrierInfo GetVulkanBarrierInfo(RESOURCE_STATE _state);
		
		
		VkCommandBuffer m_buffer{ VK_NULL_HANDLE };
	};
}
