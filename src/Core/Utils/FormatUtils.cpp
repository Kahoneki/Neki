#include "FormatUtils.h"

namespace NK
{

	std::string FormatUtils::GetSizeString(std::size_t _numBytes)
	{
		if (_numBytes / (1024 * 1024 * 1024) != 0)	{ return std::to_string(_numBytes / (1024 * 1024 * 1024)) + "GiB"; }
		if (_numBytes / (1024 * 1024) != 0)			{ return std::to_string(_numBytes / (1024 * 1024)) + "MiB"; }
		if (_numBytes / 1024 != 0)					{ return std::to_string(_numBytes / 1024) + "KiB"; }
		else										{ return std::to_string(_numBytes) + "B"; }
	}

}