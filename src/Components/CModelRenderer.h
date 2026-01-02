#pragma once

#include <portable-file-dialogs.h>

#include "CImGuiInspectorRenderable.h"

#include <Graphics/GPUUploader.h>


namespace NK
{

	struct CModelRenderer final : public CImGuiInspectorRenderable
	{
		friend class ModelVisibilityLayer;
		friend class RenderLayer;


	public:
		[[nodiscard]] inline std::string GetModelPath() const { return modelPath; }
		
		inline void SetModelPath(const std::string& _path)
		{
			if (_path == modelPath) { return; }
			filePathNotFoundError = !std::filesystem::exists(_path);
			if (!filePathNotFoundError)
			{
				nonNkModelError = (std::filesystem::path(_path).extension() != ".nkmodel");
			}
			if (!filePathNotFoundError && !nonNkModelError)
			{
				modelPath = _path;
				modelPathDirty = true;
			}
		}
		
		[[nodiscard]] inline static std::string GetStaticName() { return "Model Renderer"; }
		
		
		//Volume in local space
		glm::vec3 localSpaceOrigin;
		glm::vec3 localSpaceHalfExtents;
		
		
	private:
		virtual inline std::string GetComponentName() const override { return "Model Renderer"; }
		virtual inline ImGuiTreeNodeFlags GetTreeNodeFlags() const override { return ImGuiTreeNodeFlags_DefaultOpen; }
		virtual inline void RenderImGuiInspectorContents(Registry& _reg) override
		{
			
			if (ImGui::Button((std::string("...##") + "Model Filepath").c_str()))
			{
				const std::vector<std::string> selection = pfd::open_file("Select File", lastAccessedFilepath, { "Neki Models", "*.nkmodel", "All Files", "*" }).result();
				if (!selection.empty())
				{
					SetModelPath(std::filesystem::relative(selection[0], NEKI_BUILD_DIR).string());
					lastAccessedFilepath = selection[0];
				}
			}
		        
			ImGui::SameLine();
			char buffer[512];
			std::strncpy(buffer, modelPath.c_str(), sizeof(buffer));
			ImGui::InputText("Model Filepath", buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly);
			
			if (filePathNotFoundError) { ImGui::Text("Filepath not found!"); }
			if (nonNkModelError) { ImGui::Text("Only .nkmodel files are supported!"); }
		}
		
		
		std::string modelPath{ "Samples/Resource-Files/nkmodels/Prefabs/Cube.nkmodel" };
		bool modelPathDirty{ false };
		bool filePathNotFoundError{ false };
		bool nonNkModelError{ false };
		
		//Non-owning pointer. The RenderLayer owns and manages the GPUModel
		GPUModel* model;
		
		bool visible{ true };
		std::uint32_t visibilityIndex{ 0xFFFFFFFF };
		
		//UI
		std::string lastAccessedFilepath{ NEKI_BUILD_DIR };
	};
	
}