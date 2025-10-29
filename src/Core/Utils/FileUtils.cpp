#include "FileUtils.h"

#include <filesystem>


namespace NK
{

	std::string FileUtils::GetFileExtension(const std::string& _filepath)
	{
		return std::filesystem::path(_filepath).extension().string();
	}

}