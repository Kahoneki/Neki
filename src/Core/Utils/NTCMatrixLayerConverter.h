#pragma once

#include <Core/Memory/Allocation.h>

#include <array>
#include <string>


namespace NK::Neural
{
	
	struct ConvertedLayer
	{
		std::vector<std::uint8_t> weightData;
		std::vector<std::uint8_t> biasData;
		std::uint32_t M;
		std::uint32_t K;
	};
	
	
	class NTCMatrixLayerConverter final
	{
	public:
		//todo: temp - remove VkDevice dependency (maybe best to just move this into VulkanDevice)
		static ConvertedLayer ConvertLayer(VkDevice _device, const void* _weightData, std::size_t _weightSize, const void* _biasData, std::size_t _biasSize, std::uint32_t _M, std::uint32_t _K);
	};
	
}