#pragma once
#include <utility>

namespace NK
{
	class EnumUtils
	{
	public:
		//Check if an enum bitfield contains a specific bit
		template<typename E>
		[[nodiscard]] inline static bool Contains(E _field, E _bit)
		{
			return (std::to_underlying(_field) & std::to_underlying(_bit));
		}
	};
}