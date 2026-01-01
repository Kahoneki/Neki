#pragma once

#include "CImGuiInspectorRenderable.h"

#include <string>
#include <portable-file-dialogs.h>



namespace NK
{

	struct CSkybox final : public CImGuiInspectorRenderable
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
		[[nodiscard]] virtual inline std::string GetComponentName() const override { return "Skybox"; }
		[[nodiscard]] virtual inline ImGuiTreeNodeFlags GetTreeNodeFlags() const override { return ImGuiTreeNodeFlags_DefaultOpen; }
		virtual inline void RenderImGuiInspectorContents(Registry& _reg) override
		{
		    auto DrawPathInput = [&](const char* _label, const std::string& _path, void (CSkybox::*_setter)(const std::string&))
		    {
		        if (ImGui::Button((std::string("...##") + _label).c_str()))
		        {
		            const std::vector<std::string> selection = pfd::open_file("Select File", lastAccessedFilepath, { "KTX Files", "*.ktx *.ktx2", "All Files", "*" }).result();
		            if (!selection.empty())
		            {
		                (this->*_setter)(std::filesystem::relative(selection[0], NEKI_BUILD_DIR).string());
		                lastAccessedFilepath = selection[0];
		            }
		        }
		        
		        ImGui::SameLine();
		        char buffer[512];
		        std::strncpy(buffer, _path.c_str(), sizeof(buffer));
		        ImGui::InputText(_label, buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly);
		    };

		    DrawPathInput("Skybox Filepath", GetSkyboxFilepath(), &CSkybox::SetSkyboxFilepath);
		    DrawPathInput("Irradiance Filepath", GetIrradianceFilepath(), &CSkybox::SetIrradianceFilepath);
		    DrawPathInput("Prefilter Filepath", GetPrefilterFilepath(), &CSkybox::SetPrefilterFilepath);
		    
		    if (ImGui::Button("Auto-Populate from Directory"))
		    {
		        const std::string folder = pfd::select_folder("Select Skybox Directory", lastAccessedFilepath).result();
		        if (!folder.empty())
		        {
		            const std::filesystem::path root(NEKI_BUILD_DIR);
		            const std::filesystem::path relFolder{ std::filesystem::relative(folder, root) };
		            
		            SetSkyboxFilepath((relFolder / "skybox.ktx").string());
		            SetIrradianceFilepath((relFolder / "irradiance.ktx").string());
		            SetPrefilterFilepath((relFolder / "prefilter.ktx").string());
		            lastAccessedFilepath = folder;
		        }
		    }
		    if (ImGui::IsItemHovered())
		    {
		        ImGui::SetTooltip("Automatically populate paths using [Selected Directory] + /skybox.ktx, /irradiance.ktx, and /prefilter.ktx");
		    }
		}
		
		
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
		
		
		//For UI
		std::string lastAccessedFilepath{ NEKI_BUILD_DIR };
	};
	
}
