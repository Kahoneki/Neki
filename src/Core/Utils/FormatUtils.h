#pragma once

#include <string>


namespace NK
{

	class FormatUtils
	{
	public:
		[[nodiscard]] static std::string GetSizeString(std::size_t _numBytes);
	};
	
}