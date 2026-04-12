#include "NTCMatrixLayerConverter.h"


namespace NK::Neural
{
	
	ConvertedLayer NTCMatrixLayerConverter::ConvertLayer(VkDevice _device, const void* _weightData, std::size_t _weightSize, const void* _biasData, std::size_t _biasSize, std::uint32_t _M, std::uint32_t _K)
	{
		const PFN_vkConvertCooperativeVectorMatrixNV pfnConvert{ reinterpret_cast<PFN_vkConvertCooperativeVectorMatrixNV>(
			vkGetDeviceProcAddr(_device, "vkConvertCooperativeVectorMatrixNV")
		) };
		if (!pfnConvert)
		{
			throw std::runtime_error("vkConvertCooperativeVectorMatrixNV not available - is VK_NV_cooperative_vector enabled?");
		}
		
		ConvertedLayer layer;
		layer.M = _M;
		layer.K = _K;

		std::size_t dstSize{};
		VkConvertCooperativeVectorMatrixInfoNV info{};
		info.sType      = VK_STRUCTURE_TYPE_CONVERT_COOPERATIVE_VECTOR_MATRIX_INFO_NV;
		info.srcSize    = _weightSize;
		info.srcData.hostAddress = _weightData;
		info.pDstSize   = &dstSize;
		info.dstData.hostAddress = nullptr;
		info.srcComponentType = VK_COMPONENT_TYPE_FLOAT16_NV;
		info.dstComponentType = VK_COMPONENT_TYPE_FLOAT16_NV;
		info.numRows    = _M;
		info.numColumns = _K;
		info.srcLayout  = VK_COOPERATIVE_VECTOR_MATRIX_LAYOUT_ROW_MAJOR_NV;
		info.dstLayout  = VK_COOPERATIVE_VECTOR_MATRIX_LAYOUT_INFERENCING_OPTIMAL_NV;
		info.srcStride  = _K * sizeof(std::uint16_t);
		pfnConvert(_device, &info);

		layer.weightData.resize(dstSize);
		info.dstData.hostAddress = layer.weightData.data();
		pfnConvert(_device, &info);

		layer.biasData.resize(_biasSize);
		memcpy(layer.biasData.data(), _biasData, _biasSize);

		return layer;
	}
	
}
