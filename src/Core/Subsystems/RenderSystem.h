#pragma once

#include <Components/CCamera.h>
#include <Components/CSkybox.h>
#include <Components/CTransform.h>
#include <Core-ECS/Registry.h>
#include <Graphics/GPUUploader.h>
#include <Graphics/Window.h>
#include <RHI/ICommandBuffer.h>
#include <RHI/IDevice.h>
#include <Types/NekiTypes.h>


namespace NK
{

	struct RenderSystemDesc
	{
		GRAPHICS_BACKEND backend{ GRAPHICS_BACKEND::NONE };
		
		bool enableMSAA{ false };
		SAMPLE_COUNT msaaSampleCount{ SAMPLE_COUNT::BIT_1 };
		
		bool enableSSAA{ false };
		std::uint32_t ssaaMultiplier{ 1 };

		WindowDesc windowDesc{ "App", glm::ivec2(800, 800) };

		std::uint32_t framesInFlight{ 3 };
	};

	
	class RenderSystem final
	{
	public:
		RenderSystem(ILogger& _logger, IAllocator& _allocator, const RenderSystemDesc& _desc);
		~RenderSystem();

		void Update(Registry& _reg);

		[[nodiscard]] inline Window* GetWindow() const { return m_window.get(); }
		
		
	private:
		//Init sub-functions
		void InitBaseResources();
		void InitCameraBuffer();
		void InitSkybox();
		void InitShadersAndPipelines();
		void InitAntiAliasingResources();

		void UpdateSkybox(CSkybox& _skybox);
		void UpdateCameraBuffer(const CCamera& _camera) const;
		static void UpdateModelMatrix(CTransform& _transform);
		
		
		//Dependency injections
		ILogger& m_logger;
		IAllocator& m_allocator;
		GRAPHICS_BACKEND m_backend;
		bool m_msaaEnabled;
		SAMPLE_COUNT m_msaaSampleCount;
		bool m_ssaaEnabled;
		std::uint32_t m_ssaaMultiplier;
		WindowDesc m_windowDesc;
		std::uint32_t m_framesInFlight;

		//Tracks the current frame (in range [0, m_framesInFlight-1])
		std::uint32_t m_currentFrame;

		//Resolution of intermediate textures if SSAA is enabled
		glm::ivec2 m_supersampleResolution;
		
		UniquePtr<IDevice> m_device;
		UniquePtr<ICommandPool> m_graphicsCommandPool;
		UniquePtr<IQueue> m_graphicsQueue;
		UniquePtr<IQueue> m_transferQueue;
		UniquePtr<GPUUploader> m_gpuUploader;
		UniquePtr<IFence> m_gpuUploaderFlushFence;
		bool m_newGPUUploaderUpload; //True if there are any new gpu uploader uploads since last frame
		UniquePtr<Window> m_window;
		UniquePtr<ISurface> m_surface;
		UniquePtr<ISwapchain> m_swapchain;
		UniquePtr<ISampler> m_linearSampler;
		std::vector<UniquePtr<ICommandBuffer>> m_graphicsCommandBuffers;
		std::vector<UniquePtr<ISemaphore>> m_imageAvailableSemaphores;
		std::vector<UniquePtr<ISemaphore>> m_renderFinishedSemaphores;
		std::vector<UniquePtr<IFence>> m_inFlightFences;

		UniquePtr<IBuffer> m_camDataBuffer;
		UniquePtr<IBufferView> m_camDataBufferView;
		void* m_camDataBufferMap;
		
		UniquePtr<ITexture> m_skyboxTexture; //Not created at startup
		UniquePtr<ITextureView> m_skyboxTextureView; //Not created at startup
		UniquePtr<IBuffer> m_skyboxVertBuffer;
		UniquePtr<IBuffer> m_skyboxIndexBuffer;
		
		UniquePtr<IShader> m_meshVertShader;
		UniquePtr<IShader> m_skyboxVertShader;
		UniquePtr<IShader> m_blinnPhongFragShader;
		UniquePtr<IShader> m_pbrFragShader;
		UniquePtr<IShader> m_skyboxFragShader;
		UniquePtr<IRootSignature> m_meshPiplineRootSignature;
		UniquePtr<IPipeline> m_blinnPhongPipeline;
		UniquePtr<IPipeline> m_pbrPipeline;
		UniquePtr<IPipeline> m_skyboxPipeline;
		
		//Anti-aliasing
		UniquePtr<ITexture> m_intermediateRenderTarget;
		UniquePtr<ITextureView> m_intermediateRenderTargetView;
		UniquePtr<ITexture> m_intermediateDepthBuffer;
		UniquePtr<ITextureView> m_intermediateDepthBufferView;

		
		//RenderSystem owns and is responsible for all GPUModels - todo: move to out-of-core rendering with HLODs
		std::unordered_map<std::string, UniquePtr<GPUModel>> m_gpuModelCache;
	};

}