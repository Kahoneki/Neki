#pragma once

#include <portable-file-dialogs.h>

#include "CImGuiInspectorRenderable.h"

#include <Core/Utils/NTCLoader.h>
#include <Graphics/GPUUploader.h>


namespace NK
{

	struct CModelRenderer final : public CImGuiInspectorRenderable
	{
		friend class ModelVisibilityLayer;
		friend class RenderLayer;


	public:
		CModelRenderer() = default;
		
		CModelRenderer(const CModelRenderer& _other)
		: localSpaceOrigin(_other.localSpaceOrigin), localSpaceHalfExtents(_other.localSpaceHalfExtents), modelPath(_other.modelPath),
		  modelPathDirty(true), meshVisible(_other.meshVisible)
		{
		}
		
		CModelRenderer& operator=(const CModelRenderer& _other)
		{
			if (this == &_other) return *this;

			modelPath = _other.modelPath;
			modelPathDirty = false;

			localSpaceOrigin = _other.localSpaceOrigin;
			localSpaceHalfExtents = _other.localSpaceHalfExtents;
			meshVisible = _other.meshVisible;
			
			return *this;
		}

		CModelRenderer(CModelRenderer&&) = default;
		CModelRenderer& operator=(CModelRenderer&&) = default;
		~CModelRenderer() override = default;
		
		
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
		
		SERIALISE_MEMBER_FUNC(modelPath, localSpaceOrigin, localSpaceHalfExtents, meshVisible);
		
		
		//Volume in local space
		glm::vec3 localSpaceOrigin{ 0,0,0 };
		glm::vec3 localSpaceHalfExtents{ 0,0,0 };
		
		
	private:
		virtual inline std::string GetComponentName() const override { return "Model Renderer"; }
		virtual inline ImGuiTreeNodeFlags GetTreeNodeFlags() const override { return ImGuiTreeNodeFlags_DefaultOpen; }
		virtual inline void RenderImGuiInspectorContents(Registry& _reg) override
		{
			
			if (ImGui::Button((std::string("...##") + "Model Filepath").c_str()))
			{
				const std::filesystem::path currentPath{ std::filesystem::current_path() };
				const std::vector<std::string> selection = pfd::open_file("Select File", lastAccessedFilepath, { "Neki Models", "*.nkmodel", "All Files", "*" }).result();
				if (!selection.empty())
				{
					SetModelPath(std::filesystem::relative(selection[0], NEKI_SOURCE_DIR).string());
					lastAccessedFilepath = selection[0];
				}
				std::filesystem::current_path(currentPath);
			}
		        
			ImGui::SameLine();
			char buffer[512];
			std::strncpy(buffer, modelPath.c_str(), sizeof(buffer));
			ImGui::InputText("Model Filepath", buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly);
			
			if (filePathNotFoundError) { ImGui::Text("Filepath not found!"); }
			if (nonNkModelError) { ImGui::Text("Only .nkmodel files are supported!"); }
		}
		
		
		std::string modelPath{ "Samples/Resource-Files/nkmodels/Prefabs/Cube/model.nkmodel" };
		bool modelPathDirty{ true };
		bool filePathNotFoundError{ false };
		bool nonNkModelError{ false };
		std::string meshDataPath{ "Samples/Resource-Files/nkmodels/Prefabs/Cube/mesh.nkmeshdata" };
		
		//Non-owning pointers. The RenderLayer owns and manages the GPUMeshes
		std::vector<GPUMesh*> meshes;
		std::vector<GPUMaterial*> materials; //parallel to `meshes`
		
		std::vector<bool> meshVisible;
		std::vector<std::uint32_t> meshVisibilityIndices;
		
		std::vector<MeshDataLoadInfo> meshDataLoadInfos;
		
		//UI
		std::string lastAccessedFilepath{ NEKI_SOURCE_DIR };
	};
	
}
