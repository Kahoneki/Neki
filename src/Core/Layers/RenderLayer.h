#pragma once

#include "ILayer.h"

#include <Components/CCamera.h>
#include <Components/CSkybox.h>
#include <Components/CTransform.h>
#include <Components/CWindow.h>
#include <Core-ECS/Registry.h>
#include <Graphics/GPUUploader.h>
#include <Graphics/RenderGraph.h>
#include <Graphics/Window.h>
#include <RHI/ICommandBuffer.h>
#include <RHI/IDevice.h>
#include <Types/NekiTypes.h>


namespace NK
{

	struct RenderLayerDesc
	{
		explicit RenderLayerDesc(const GRAPHICS_BACKEND _backend, const bool _enableMSAA, const SAMPLE_COUNT _msaaSampleCount, const bool _enableSSAA, const std::uint32_t _ssaaMultiplier, Window* _window, const std::uint32_t _framesInFlight, const std::uint32_t _maxModels)
		: backend(_backend), enableMSAA(_enableMSAA), msaaSampleCount(_msaaSampleCount), enableSSAA(_enableSSAA), ssaaMultiplier(_ssaaMultiplier), window(_window), framesInFlight(_framesInFlight), maxModels(_maxModels) {}

		RenderLayerDesc() {}

		GRAPHICS_BACKEND backend{ GRAPHICS_BACKEND::VULKAN };
		
		bool enableMSAA{ false };
		SAMPLE_COUNT msaaSampleCount{ SAMPLE_COUNT::BIT_1 };
		
		bool enableSSAA{ false };
		std::uint32_t ssaaMultiplier{ 1 };

		Window* window{ nullptr };

		std::uint32_t framesInFlight{ 3 };

		std::uint32_t maxModels{ 10'000 };
	};

	
	class RenderLayer final : public ILayer
	{
	public:
		explicit RenderLayer(Registry& _reg, const RenderLayerDesc& _desc);
		virtual ~RenderLayer() override;

		virtual void Update() override;
		
		
	private:
		//Init sub-functions
		void InitBaseResources();
		void InitCameraBuffer();
		void InitLightCameraBuffer();
		void InitModelMatricesBuffer();
		void InitModelVisibilityBuffers();
		void InitCube();
		void InitScreenQuad();
		void InitShadersAndPipelines();
		void InitModelVisibilityPipeline();
		void InitShadowPipeline();
		void InitSkyboxPipeline();
		void InitGraphicsPipelines();
		void InitPostprocessPipeline();
		void InitRenderGraphs();
		void InitScreenResources();

		void UpdateSkybox(CSkybox& _skybox);
		void UpdateCameraBuffer(const CCamera& _camera) const;
		void UpdateLightCameraBuffer();
		void UpdateModelMatricesBuffer();
		
		
		//Dependency injections
		IAllocator& m_allocator;
		const RenderLayerDesc m_desc;

		//Tracks the current frame (in range [0, m_framesInFlight-1])
		std::uint32_t m_currentFrame;

		//Resolution of intermediate textures if SSAA is enabled
		glm::ivec2 m_supersampleResolution;
		
		UniquePtr<IDevice> m_device;
		std::vector<UniquePtr<ICommandPool>> m_graphicsCommandPools;
		UniquePtr<IQueue> m_graphicsQueue;
		UniquePtr<IQueue> m_transferQueue;
		UniquePtr<GPUUploader> m_gpuUploader;
		UniquePtr<IFence> m_gpuUploaderFlushFence;
		bool m_newGPUUploaderUpload; //True if there are any new gpu uploader uploads since last frame
		UniquePtr<ISurface> m_surface;
		UniquePtr<ISwapchain> m_swapchain;
		std::uint32_t m_swapchainImageIndex;
		UniquePtr<ISampler> m_linearSampler;
		std::vector<UniquePtr<ICommandBuffer>> m_graphicsCommandBuffers;
		std::vector<UniquePtr<ISemaphore>> m_imageAvailableSemaphores;
		std::vector<UniquePtr<ISemaphore>> m_renderFinishedSemaphores;
		std::vector<UniquePtr<IFence>> m_inFlightFences;

		UniquePtr<IBuffer> m_camDataBuffer;
		UniquePtr<IBufferView> m_camDataBufferView;
		void* m_camDataBufferMap;

		struct LightCameraShaderData
		{
			glm::mat4 viewMat;
			glm::mat4 projMat;
		};
		UniquePtr<IBuffer> m_lightCamDataBuffer;
		UniquePtr<IBufferView> m_lightCamDataBufferView;
		void* m_lightCamDataBufferMap;

		//Stores the model matrices for all models - regardless of whether the model is loaded or not
		//(parallel to m_modelVisibilityBuffers)
		//(one for each frame in flight)
		std::vector<UniquePtr<IBuffer>> m_modelMatricesBuffers;
		std::vector<UniquePtr<IBufferView>> m_modelMatricesBufferViews;
		std::vector<void*> m_modelMatricesBufferMaps;

		//Stores a 32-bit uint for every model in the scene indicating whether it's visible or not - regardless of whether the model is loaded or not
		//(all vectors parallel to m_modelMatricesBuffers)
		//(one for each frame in flight)
		std::vector<UniquePtr<IBuffer>> m_modelVisibilityBuffers;
		std::vector<UniquePtr<IBufferView>> m_modelVisibilityBufferViews;
		std::vector<void*> m_modelVisibilityBufferMaps;
		
		UniquePtr<ITexture> m_skyboxTexture; //Not created at startup
		UniquePtr<ITextureView> m_skyboxTextureView; //Not created at startup
		UniquePtr<IBuffer> m_cubeVertBuffer;
		UniquePtr<IBuffer> m_cubeIndexBuffer;

		struct ScreenQuadVertex
		{
			glm::vec3 pos;
			glm::vec2 uv;
		};
		UniquePtr<IBuffer> m_screenQuadVertBuffer;
		UniquePtr<IBuffer> m_screenQuadIndexBuffer;
		
		UniquePtr<IShader> m_shadowVertShader;
		UniquePtr<IShader> m_meshVertShader;
		UniquePtr<IShader> m_skyboxVertShader;
		UniquePtr<IShader> m_screenQuadVertShader;
		UniquePtr<IShader> m_modelVisibilityVertShader;

		UniquePtr<IShader> m_shadowFragShader;
		UniquePtr<IShader> m_skyboxFragShader;
		UniquePtr<IShader> m_blinnPhongFragShader;
		UniquePtr<IShader> m_pbrFragShader;
		UniquePtr<IShader> m_postprocessFragShader;
		UniquePtr<IShader> m_modelVisibilityFragShader;

		struct ModelVisibilityPassPushConstantData
		{
			ResourceIndex camDataBufferIndex;
			ResourceIndex modelMatricesBufferIndex;
			ResourceIndex modelVisibilityBufferIndex;
		};
		UniquePtr<IRootSignature> m_modelVisibilityPassRootSignature;
		
		struct ShadowPassPushConstantData
		{
			glm::mat4 modelMat;
			ResourceIndex lightCamDataBufferIndex;
		};
		UniquePtr<IRootSignature> m_shadowPassRootSignature;
		
		struct MeshPassPushConstantData
		{
			glm::mat4 modelMat;
			ResourceIndex camDataBufferIndex;
			ResourceIndex lightCamDataBufferIndex;
			ResourceIndex shadowMapIndex;
			ResourceIndex skyboxCubemapIndex;
			ResourceIndex materialBufferIndex;
			SamplerIndex samplerIndex;
		};
		UniquePtr<IRootSignature> m_meshPassRootSignature;

		struct PostprocessPassPushConstantData
		{
			ResourceIndex shadowMapIndex;
			ResourceIndex sceneColourIndex;
			ResourceIndex sceneDepthIndex;
			SamplerIndex samplerIndex;
		};
		UniquePtr<IRootSignature> m_postprocessPassRootSignature;
		
		UniquePtr<IPipeline> m_modelVisibilityPipeline;
		UniquePtr<IPipeline> m_shadowPipeline;
		UniquePtr<IPipeline> m_skyboxPipeline;
		UniquePtr<IPipeline> m_blinnPhongPipeline;
		UniquePtr<IPipeline> m_pbrPipeline;
		UniquePtr<IPipeline> m_postprocessPipeline;

		UniquePtr<RenderGraph> m_meshRenderGraph;
		RenderGraph* m_activeRenderGraph;

		
		//Screen Resources
		UniquePtr<ITexture> m_shadowMap;
		UniquePtr<ITexture> m_shadowMapMSAA;
		UniquePtr<ITexture> m_shadowMapSSAA;
		UniquePtr<ITextureView> m_shadowMapDSV;
		UniquePtr<ITextureView> m_shadowMapSRV;
		UniquePtr<ITextureView> m_shadowMapMSAADSV;
		UniquePtr<ITextureView> m_shadowMapMSAASRV;
		UniquePtr<ITextureView> m_shadowMapSSAADSV;
		UniquePtr<ITextureView> m_shadowMapSSAASRV;
		
		UniquePtr<ITexture> m_sceneColour;
		UniquePtr<ITexture> m_sceneColourMSAA;
		UniquePtr<ITexture> m_sceneColourSSAA;
		UniquePtr<ITextureView> m_sceneColourRTV;
		UniquePtr<ITextureView> m_sceneColourSRV;
		UniquePtr<ITextureView> m_sceneColourMSAARTV;
		UniquePtr<ITextureView> m_sceneColourMSAASRV;
		UniquePtr<ITextureView> m_sceneColourSSAARTV;
		UniquePtr<ITextureView> m_sceneColourSSAASRV;

		UniquePtr<ITexture> m_sceneDepth;
		UniquePtr<ITexture> m_sceneDepthMSAA;
		UniquePtr<ITexture> m_sceneDepthSSAA;
		UniquePtr<ITextureView> m_sceneDepthDSV;
		UniquePtr<ITextureView> m_sceneDepthSRV;
		UniquePtr<ITextureView> m_sceneDepthSSAADSV;
		UniquePtr<ITextureView> m_sceneDepthSSAASRV;
		UniquePtr<ITextureView> m_sceneDepthMSAADSV;
		UniquePtr<ITextureView> m_sceneDepthMSAASRV;

		
		//Stores the model matrices for all models - regardless of whether the model is loaded or not
		//(used to populate m_modelMatricesBuffer)
		std::vector<glm::mat4> m_modelMatrices;
		//(parallel to m_modelMatrices)
		//(one for each frame in flight)
		std::vector<std::vector<Entity>> m_modelMatricesEntitiesLookups;
		
		
		//RenderLayer owns and is responsible for all GPUModels - todo: move to out-of-core rendering with HLODs
		std::unordered_map<std::string, UniquePtr<GPUModel>> m_gpuModelCache;

		//A model can't be unloaded until it's done being used by the GPU, keep track of all models we need to unload for each frame in flight
		//Once the appropriate fence has been signalled, unload all models in the corresponding vector
		//Note: a vector is used here instead of a queue to allow for searching the queue for a specific model
		std::vector<std::vector<std::string>> m_modelUnloadQueues;
	};

}