#pragma once

#include <Core/Memory/Allocation.h>

#include <array>
#include <string>


namespace NK::Neural
{
	
	struct NTCHeader
	{
		std::uint32_t version;
		std::uint32_t baseImageRes;
		std::uint32_t numMips;
		std::uint32_t qualityValue;
		std::uint32_t hiddenNeurons;
		std::uint32_t g0Channels;
		std::uint32_t g1Channels;
		std::uint32_t g0QuantLevels;
		std::uint32_t g1QuantLevels;
		std::uint32_t numLinearLayers;
		std::uint32_t numOctaves;
		std::uint32_t tileSize;
	};
	
	struct NTCLatentTexture
	{
		std::uint32_t res;
		std::uint32_t numElements; //Number of elements before packing
		std::uint32_t numElementsPacked; //Number of elements after packing
		unsigned char* data;
	};
	
	struct NTCFeatureLevel
	{
		NTCLatentTexture g0;
		NTCLatentTexture g1;
	};
	
	struct NTCLinearLayer
	{
		std::uint32_t inFeatures;
		std::uint32_t outFeatures;
		
		//Weights and biases were saved as a contiguous fp16 block
		std::vector<std::uint16_t> weights; //Size: inFeatures * outFeatures (flattened for contiguity)
		std::vector<std::uint16_t> biases; //Size: outFeatures
	};
	
	struct NTCModel
	{
		NTCHeader header;
		std::array<NTCFeatureLevel, 4> featureLevels; //4 feature levels
		std::vector<NTCLinearLayer> mlp; //Size: header.numLinearLayers
	};

	
	class NTCLoader final
	{
	public:
		~NTCLoader();
		
		[[nodiscard]] static NTCModel* LoadMaterial(const std::string& _filepath);
		static void FreeMaterial(NTCModel* _modelData);
		static void ClearCache();
		
		
	private:
		//To avoid unnecessary duplicate loads
		static std::unordered_map<std::string, UniquePtr<NTCModel>> m_filepathToNTCModelCache;
	};
	
}