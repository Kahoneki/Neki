#pragma once

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
		
		inline void SetModelPath(const std::string& _path) { modelPathDirty = (_path != modelPath); modelPath = _path; }
		
		[[nodiscard]] inline static std::string GetStaticName() { return "Model Renderer"; }
		
		
		//Volume in local space
		glm::vec3 localSpaceOrigin;
		glm::vec3 localSpaceHalfExtents;
		
		
	private:
		virtual inline std::string GetComponentName() const override { return "Model Renderer"; }
		virtual inline ImGuiTreeNodeFlags GetTreeNodeFlags() const override { return ImGuiTreeNodeFlags_DefaultOpen; }
		virtual inline void RenderImGuiInspectorContents(Registry& _reg) override
		{
			char buf[256];
			std::strncpy(buf, modelPath.c_str(), sizeof(buf));
			buf[sizeof(buf) - 1] = '\0';

			if (ImGui::InputText("Filepath", buf, sizeof(buf), ImGuiInputTextFlags_EnterReturnsTrue))
			{
				SetModelPath(buf);
				
				filePathNotFoundError = !std::filesystem::exists(modelPath);
				if (!filePathNotFoundError)
				{
					nonNkModelError = (std::filesystem::path(modelPath).extension() != ".nkmodel");
				}
			}
			
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
	};
	
}