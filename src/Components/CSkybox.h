#pragma once

#include "CImGuiInspectorRenderable.h"

#include <string>



namespace NK
{

	struct CSkybox final
	{
		friend class RenderLayer;
		
		
	public:
		[[nodiscard]] inline std::string GetSkyboxFilepath() { return skyboxFilepath; }
		[[nodiscard]] inline std::string GetIrradianceFilepath() { return irradianceFilepath; }
		[[nodiscard]] inline std::string GetPrefilterFilepath() { return prefilterFilepath; }

		inline void SetSkyboxFilepath(const std::string& _val)
		{
			if (_val == skyboxFilepath) { return; }
			skyboxFilepath = _val;
			skyboxFilepathDirty = true;
		}
		inline void SetIrradianceFilepath(const std::string& _val)
		{
			if (_val == irradianceFilepath) { return; }
			irradianceFilepath = _val;
			irradianceFilepathDirty = true;
		}
		inline void SetPrefilterFilepath(const std::string& _val)
		{
			if (_val == prefilterFilepath) { return; }
			prefilterFilepath = _val;
			prefilterFilepathDirty = true;
		}
		
		
	private:
		//Should be an R16G16B16A16_SFLOAT .ktx cubemap
		std::string skyboxFilepath;
		//Should be an R16G16B16A16_SFLOAT .ktx cubemap
		std::string irradianceFilepath;
		//Should be an R16G16B16A16_SFLOAT .ktx cubemap
		std::string prefilterFilepath;

		//True if skyboxFilepath has been modified and the skybox hasn't yet been updated by RenderLayer
		bool skyboxFilepathDirty{ true };
		//True if irradianceFilepath has been modified and the skybox hasn't yet been updated by RenderLayer
		bool irradianceFilepathDirty{ true };
		//True if prefilterFilepath has been modified and the skybox hasn't yet been updated by RenderLayer
		bool prefilterFilepathDirty{ true };
	};
	
}
