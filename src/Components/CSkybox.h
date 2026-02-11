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
			skyboxFilepathNotFoundError = !std::filesystem::exists(_val);
			if (!skyboxFilepathNotFoundError)
			{
				skyboxFileExtensionNotKTXError = (std::filesystem::path(_val).extension() != ".ktx");
			}
			if (!skyboxFilepathNotFoundError && !skyboxFileExtensionNotKTXError)
			{
				skyboxFilepath = _val;
				skyboxFilepathDirty = true;
			}
		}
		inline void SetIrradianceFilepath(const std::string& _val)
		{
			if (_val == irradianceFilepath) { return; }
			irradianceFilepathNotFoundError = !std::filesystem::exists(_val);
			if (!irradianceFilepathNotFoundError)
			{
				irradianceFileExtensionNotKTXError = (std::filesystem::path(_val).extension() != ".ktx");
			}
			if (!irradianceFilepathNotFoundError && !irradianceFileExtensionNotKTXError)
			{
				irradianceFilepath = _val;
				irradianceFilepathDirty = true;
			}
		}
		inline void SetPrefilterFilepath(const std::string& _val)
		{
			if (_val == prefilterFilepath) { return; }
			prefilterFilepathNotFoundError = !std::filesystem::exists(_val);
			if (!prefilterFilepathNotFoundError)
			{
				prefilterFileExtensionNotKTXError = (std::filesystem::path(_val).extension() != ".ktx");
			}
			if (!prefilterFilepathNotFoundError && !prefilterFileExtensionNotKTXError)
			{
				prefilterFilepath = _val;
				prefilterFilepathDirty = true;
			}
		}
		
		[[nodiscard]] inline static std::string GetStaticName() { return "Skybox"; }
		
		SERIALISE_MEMBER_FUNC(skyboxFilepath, irradianceFilepath, prefilterFilepath)
		
		
	private:
		[[nodiscard]] virtual inline std::string GetComponentName() const override { return GetStaticName(); }
		[[nodiscard]] virtual inline ImGuiTreeNodeFlags GetTreeNodeFlags() const override { return ImGuiTreeNodeFlags_DefaultOpen; }
		virtual inline void RenderImGuiInspectorContents(Registry& _reg) override
		{
		    auto DrawPathInput = [&](const char* _label, const std::string& _path, void (CSkybox::*_setter)(const std::string&))
		    {
		        if (ImGui::Button((std::string("...##") + _label).c_str()))
		        {
		        	const std::filesystem::path currentPath{ std::filesystem::current_path() };
		            const std::vector<std::string> selection = pfd::open_file("Select File", lastAccessedFilepath, { "KTX Files", "*.ktx *.ktx2", "All Files", "*" }).result();
		            if (!selection.empty())
		            {
		                (this->*_setter)(std::filesystem::relative(selection[0], NEKI_SOURCE_DIR).string());
		                lastAccessedFilepath = selection[0];
		            }
		        	std::filesystem::current_path(currentPath);
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
		    	const std::filesystem::path currentPath{ std::filesystem::current_path() };
		        const std::string folder = pfd::select_folder("Select Skybox Directory", lastAccessedFilepath).result();
		        if (!folder.empty())
		        {
		            const std::filesystem::path root(NEKI_SOURCE_DIR);
		            const std::filesystem::path relFolder{ std::filesystem::relative(folder, root) };
		            
		            SetSkyboxFilepath((relFolder / "skybox.ktx").string());
		            SetIrradianceFilepath((relFolder / "irradiance.ktx").string());
		            SetPrefilterFilepath((relFolder / "prefilter.ktx").string());
		            lastAccessedFilepath = folder;
		        }
		    	std::filesystem::current_path(currentPath);
		    }
		    if (ImGui::IsItemHovered())
		    {
		        ImGui::SetTooltip("Automatically populate paths using [Selected Directory] + /skybox.ktx, /irradiance.ktx, and /prefilter.ktx");
		    }
			
			
			if (skyboxFilepathNotFoundError)		{ ImGui::Text("Skybox Filepath Not Found!"); }
			if (irradianceFilepathNotFoundError)	{ ImGui::Text("Irradiance Filepath Not Found!"); }
			if (prefilterFilepathNotFoundError)		{ ImGui::Text("Prefilter Filepath Not Found!"); }
			
			if (skyboxFileExtensionNotKTXError)		{ ImGui::Text("Skybox Extension Must Be .ktx!"); }
			if (irradianceFileExtensionNotKTXError)	{ ImGui::Text("Irradiance Extension Must Be .ktx!"); }
			if (prefilterFileExtensionNotKTXError)	{ ImGui::Text("Prefilter Extension Must Be .ktx!"); }
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
		
		bool skyboxFilepathNotFoundError{ false };
		bool skyboxFileExtensionNotKTXError{ false };
		bool irradianceFilepathNotFoundError{ false };
		bool irradianceFileExtensionNotKTXError{ false };
		bool prefilterFilepathNotFoundError{ false };
		bool prefilterFileExtensionNotKTXError{ false };
		
		
		//UI
		std::string lastAccessedFilepath{ NEKI_SOURCE_DIR };
	};
	
}
