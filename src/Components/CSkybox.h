#pragma once

#include <string>


namespace NK
{

	struct CSkybox final
	{
		friend class RenderLayer;
		
		
	public:
		[[nodiscard]] inline std::string GetTextureDirectory() { return textureDirectory; }
		[[nodiscard]] inline std::string GetFileExtension() { return fileExtension; }

		inline void SetTextureDirectory(const std::string& _val) { textureDirectory = _val; dirty = true; }
		inline void SetFileExtension(const std::string& _val) { fileExtension = _val; dirty = true; }
		
		
	private:
		//Inside textureDirectory, there should be 6 images named:
		//- right.fileExtension
		//- left.fileExtension,
		//- top.fileExtension,
		//- bottom.fileExtension,
		//- front.fileExtension,
		//- back.fileExtension,
		std::string textureDirectory;
		std::string fileExtension;

		//True if textureDirectory or fileExtension has been modified and the skybox hasn't yet been updated by RenderLayer
		bool dirty{ true };
	};
	
}