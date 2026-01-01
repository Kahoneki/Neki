#pragma once

#include "ILayer.h"

#include <Components/CCamera.h>
#include <Components/CLight.h>
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

#include <ImGuizmo.h>


namespace NK
{

	struct RenderLayerDesc
	{
		explicit RenderLayerDesc(const GRAPHICS_BACKEND _backend, const bool _enableMSAA, const SAMPLE_COUNT _msaaSampleCount, const bool _enableSSAA, const std::uint32_t _ssaaMultiplier, Window* _window, const std::uint32_t _framesInFlight, const std::uint32_t _maxModels, const std::uint32_t _maxLights)
			: backend(_backend), enableMSAA(_enableMSAA), msaaSampleCount(_msaaSampleCount), enableSSAA(_enableSSAA), ssaaMultiplier(_ssaaMultiplier), window(_window), framesInFlight(_framesInFlight), maxModels(_maxModels), maxLights(_maxLights) {}

		RenderLayerDesc() {}

		GRAPHICS_BACKEND backend{ GRAPHICS_BACKEND::VULKAN };

		bool enableMSAA{ false };
		SAMPLE_COUNT msaaSampleCount{ SAMPLE_COUNT::BIT_1 };

		bool enableSSAA{ false };
		std::uint32_t ssaaMultiplier{ 1 };

		Window* window{ nullptr };

		std::uint32_t framesInFlight{ 3 };

		std::uint32_t maxModels{ 10'000 };
		std::uint32_t maxLights{ 16 };
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
		void InitImGui();
		void InitCameraBuffers();
		void InitLightDataBuffer();
		void InitModelMatricesBuffer();
		void InitModelVisibilityBuffers();
		void InitCube();
		void InitScreenQuad();

		void InitShadersAndPipelines();
		void InitModelVisibilityPipeline();
		void InitShadowPipeline();
		void InitSkyboxPipeline();
		void InitGraphicsPipelines();
		void InitPrefixSumPipeline();
		void InitPostprocessPipeline();

		void InitRenderGraphs();
		void InitScreenResources();
		void InitBRDFLut();

		//Automatically determines whether to make a 2d texture or a cubemap based on light type
		void InitShadowMapForLight(const CLight& _light);

		void PreAppUpdate();
		void UpdateImGui(const CCamera& _camera);
		void DrawImGuiHierarchy(CTransform& _transform); //Draw the entire hierarchy for a single entity (its node and all of its children's nodes)
		void DrawImGuiHierarchyNode(CTransform& _transform); //Draw the node for a single entity in a hierarchy
		void PostAppUpdate();

		void UpdateSkybox(CSkybox& _skybox);
		void UpdateCameraBuffer(const CCamera& _camera) const;
		void UpdateLightDataBuffer();
		void UpdateModelMatricesBuffer();

		static glm::mat4 GetPointLightViewMatrix(const glm::vec3& _lightPos, const std::size_t _faceIndex);
		
		void OnEntityDestroy(const EntityDestroyEvent& _event);


		//Dependency injections
		IAllocator& m_allocator;
		const RenderLayerDesc m_desc;

		//Tracks the current frame (in range [0, m_framesInFlight-1])
		std::uint32_t m_currentFrame;
		//Tracks the current global frame (in range [0,N])
		std::uint64_t m_globalFrame;

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
		UniquePtr<ISampler> m_brdfLUTSampler;
		std::vector<UniquePtr<ICommandBuffer>> m_graphicsCommandBuffers;
		std::vector<UniquePtr<ISemaphore>> m_imageAvailableSemaphores;
		std::vector<UniquePtr<ISemaphore>> m_renderFinishedSemaphores;
		std::vector<UniquePtr<IFence>> m_inFlightFences;

		//(one for each frame in flight)
		std::vector<UniquePtr<IBuffer>> m_camDataBuffers;
		std::vector<UniquePtr<IBufferView>> m_camDataBufferViews;
		std::vector<void*> m_camDataBufferMaps;
		//To avoid temporal artifacts because last frame's depth buffer is being used for the model visibility pass
		//(one for each frame in flight)
		std::vector<UniquePtr<IBuffer>> m_camDataBuffersPreviousFrame;
		std::vector<UniquePtr<IBufferView>> m_camDataBufferPreviousFrameViews;
		std::vector<void*> m_camDataBufferPreviousFrameMaps;


		struct alignas(16) LightShaderData
		{
			//Do not reorder - aligned for hlsl
			glm::mat4 viewProjMat;
			glm::vec3 colour;
			float intensity;
			glm::vec3 position;
			LIGHT_TYPE type;
			glm::vec3 direction;
			float innerAngle;
			float outerAngle;
			float constantAttenuation;
			float linearAttenuation;
			float quadraticAttenuation;
			std::uint32_t shadowMapIndex;
			float padding[3];
		};
		std::vector<LightShaderData> m_cpuLightData;
		UniquePtr<IBuffer> m_lightDataBuffer;
		UniquePtr<IBufferView> m_lightDataBufferView; //SRV
		void* m_lightDataBufferMap;

		UniquePtr<FreeListAllocator> m_visibilityIndexAllocator;
		struct alignas(16) ModelMatrixShaderData
		{
			glm::mat4 modelMatrix;
			std::uint32_t visibilityIndex;
			std::uint32_t padding[3];
		};
		//Stores the model matrices for all models - regardless of whether the model is loaded or not
		//(parallel to m_modelVisibilityBuffers)
		//(one for each frame in flight)
		std::vector<UniquePtr<IBuffer>> m_modelMatricesBuffers;
		std::vector<UniquePtr<IBufferView>> m_modelMatricesBufferViews;
		std::vector<void*> m_modelMatricesBufferMaps;

		//Stores a 32-bit uint for every model in the scene indicating whether it's visible or not - regardless of whether the model is loaded or not
		//(all vectors parallel to m_modelMatricesBuffers)
		//(one for each frame in flight)
		std::vector<UniquePtr<IBuffer>> m_modelVisibilityDeviceBuffers;
		std::vector<UniquePtr<IBuffer>> m_modelVisibilityReadbackBuffers;
		std::vector<UniquePtr<IBufferView>> m_modelVisibilityDeviceBufferViews;
		std::vector<void*> m_modelVisibilityReadbackBufferMaps;

		//(one for each frame in flight)
		std::size_t m_skyboxDirtyCounter;
		std::size_t m_irradianceDirtyCounter;
		std::size_t m_prefilterDirtyCounter;
		std::vector<UniquePtr<ITexture>> m_skyboxTextures; //Not created at startup
		std::vector<UniquePtr<ITextureView>> m_skyboxTextureViews; //Not created at startup
		std::vector<UniquePtr<ITexture>> m_irradianceMaps;
		std::vector<UniquePtr<ITextureView>> m_irradianceMapViews;
		std::vector<UniquePtr<ITexture>> m_prefilterMaps;
		std::vector<UniquePtr<ITextureView>> m_prefilterMapViews;
		UniquePtr<ITexture> m_brdfLUT;
		UniquePtr<ITextureView> m_brdfLUTView;
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
		UniquePtr<IShader> m_prefixSumCompShader;

		struct ModelVisibilityPassPushConstantData
		{
			ResourceIndex camDataBufferIndex;
			ResourceIndex modelMatricesBufferIndex;
			ResourceIndex modelVisibilityBufferIndex;
		};
		UniquePtr<IRootSignature> m_modelVisibilityPassRootSignature;

		struct ShadowPassPushConstantData
		{
			glm::mat4 viewProjMat;
			glm::mat4 modelMat;
		};
		UniquePtr<IRootSignature> m_shadowPassRootSignature;

		struct MeshPassPushConstantData
		{
			glm::mat4 modelMat;
			ResourceIndex camDataBufferIndex;
			std::uint32_t numLights;
			ResourceIndex lightDataBufferIndex;

			ResourceIndex skyboxCubemapIndex;
			ResourceIndex irradianceCubemapIndex;
			ResourceIndex prefilterCubemapIndex;
			ResourceIndex brdfLUTIndex;
			SamplerIndex brdfLUTSamplerIndex;

			ResourceIndex materialBufferIndex;
			SamplerIndex samplerIndex;
		};
		UniquePtr<IRootSignature> m_meshPassRootSignature;

		struct PrefixSumPassPushConstantData
		{
			ResourceIndex inputTextureIndex;
			ResourceIndex outputTextureIndex;

			//Dimensions of the texture being processed in this specific pass.
			//Pass 1: (Width, Height)
			//Pass 2: (Height, Width) - pass 1 transposes the output to create the SAT
			glm::uvec2 dimensions;
		};
		UniquePtr<IRootSignature> m_prefixSumPassRootSignature;

		struct PostprocessPassPushConstantData
		{
			ResourceIndex sceneColourIndex;
			ResourceIndex sceneDepthIndex;
			ResourceIndex satTextureIndex;
			SamplerIndex samplerIndex;

			float nearPlane;
			float farPlane;
			float focalDistance;
			float focalDepth;
			float maxBlurRadius;
			std::uint32_t dofDebugMode;

			float acesExposure;
		};
		UniquePtr<IRootSignature> m_postprocessPassRootSignature;

		UniquePtr<IPipeline> m_modelVisibilityPipeline;
		UniquePtr<IPipeline> m_shadowPipeline;
		UniquePtr<IPipeline> m_skyboxPipeline;
		UniquePtr<IPipeline> m_blinnPhongPipeline;
		UniquePtr<IPipeline> m_pbrPipeline;
		UniquePtr<IPipeline> m_prefixSumPipeline;
		UniquePtr<IPipeline> m_postprocessPipeline;

		UniquePtr<RenderGraph> m_meshRenderGraph;
		RenderGraph* m_activeRenderGraph;


		//Shadow maps
		static constexpr glm::ivec2 m_shadowMapBaseResolution{ 1024, 1024 };
		std::vector<UniquePtr<ITexture>> m_shadowMaps2D;
		std::vector<UniquePtr<ITextureView>> m_shadowMap2DDSVs;
		std::vector<UniquePtr<ITextureView>> m_shadowMap2DSRVs;

		glm::mat4 m_pointLightProjMatrix;
		std::vector<UniquePtr<ITexture>> m_shadowMapsCube;
		std::vector<std::vector<UniquePtr<ITextureView>>> m_shadowMapCube_FaceDSVs; //Each inner vector is 6 DSVs (1 for each face in the shadow cubemap)
		std::vector<UniquePtr<ITextureView>> m_shadowMapCubeSRVs;


		//Screen Resources
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

		//For Depth of Field
		//Double buffered to ping pong
		UniquePtr<ITexture> m_satIntermediate;
		UniquePtr<ITexture> m_satFinal;
		UniquePtr<ITextureView> m_satIntermediateUAV;
		UniquePtr<ITextureView> m_satFinalUAV;
		UniquePtr<ITextureView> m_satIntermediateSRV;
		UniquePtr<ITextureView> m_satFinalSRV;


		//Stores the model matrices for all models - regardless of whether the model is loaded or not
		//(used to populate m_modelMatricesBuffer)
		std::vector<glm::mat4> m_modelMatrices;
		//(parallel to m_modelMatrices)
		//(one for each frame in flight)
		std::vector<std::vector<Entity>> m_modelMatricesEntitiesLookups;


		//RenderLayer owns and is responsible for all GPUModels - todo: move to out-of-core rendering with HLODs
		std::unordered_map<std::string, UniquePtr<GPUModel>> m_gpuModelCache;

		//For each model, this tracks how many CModelRenderers use it - so we know when to unload it
		std::unordered_map<std::string, std::uint32_t> m_gpuModelReferenceCounter;

		//A model can't be unloaded until it's done being used by the GPU, keep track of all models we need to unload and the global frame count on which it is safe to do so
		//Once the appropriate fence has been signalled, unload all models in the corresponding vector
		std::unordered_map<std::uint64_t, std::vector<std::string>> m_modelUnloadQueue;
		
		struct DeferredTextureDeletions
		{
			std::vector<UniquePtr<ITexture>> textures;
			std::vector<UniquePtr<ITextureView>> views;
		};
		std::unordered_map<std::uint64_t, DeferredTextureDeletions> m_textureDeletionQueue;

		EventSubscriptionID m_entityDestroyEventSubscriptionID;
		
		bool m_firstFrame;
		
		
		//UI
		Entity m_entityPendingDeletion{ UINT32_MAX };
		ImGuizmo::OPERATION m_currentGizmoOp{ ImGuizmo::TRANSLATE };
		ImGuizmo::MODE m_currentGizmoMode{ ImGuizmo::WORLD };
		CCamera* m_activeCamera{ nullptr };
	};

}