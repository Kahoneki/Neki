#pragma once

#include <string>


namespace NK
{

	class FileUtils
	{
	public:
		[[nodiscard]] static std::string GetFileExtension(const std::string& _filepath);
	};
	
}