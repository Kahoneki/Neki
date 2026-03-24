#include "NTCLoader.h"


namespace NK::Neural
{
	
	std::unordered_map<std::string, UniquePtr<NTCModel>> NTCLoader::m_filepathToNTCModelCache;


	NTCLoader::~NTCLoader()
	{
		ClearCache();
	}

	
	
	NTCModel* NTCLoader::LoadMaterial(const std::string& _filepath)
	{
		const std::unordered_map<std::string, UniquePtr<NTCModel>>::iterator it{ m_filepathToNTCModelCache.find(_filepath) };
		if (it != m_filepathToNTCModelCache.end())
		{
			return it->second.get();
		}
		
		FILE* fp{ fopen(_filepath.c_str(), "rb") };
		if (!fp) { throw std::invalid_argument("NTCLoader::LoadMaterial - provided _filepath is invalid."); }
		char magic[4];
		fread(magic, 1, 4, fp);
		if (memcmp(magic, "NTC\0", 4) != 0)
		{
			fclose(fp);
			throw std::invalid_argument("NTCLoader::LoadMaterial - provided _filepath did not contain NTC\\0 header.");
		}
		auto r{ [&]()
		{
			std::uint32_t v;
			fread(&v, 4, 1, fp);
			return v;
		} };
		
		NTCModel model{};
		model.header.version = r();
		model.header.baseImageRes = r();
		model.header.numMips = r();
		model.header.qualityValue = r();
		model.header.hiddenNeurons = r();
		model.header.g0Channels = r();
		model.header.g1Channels = r();
		model.header.g0QuantLevels = r();
		model.header.g1QuantLevels = r();
		model.header.numLinearLayers = r();
		model.header.numOctaves = r();
		model.header.tileSize = r();
		
		for (NTCFeatureLevel& fl : model.featureLevels)
		{
			fl.g0.res = r();
			fl.g0.numElements = r();
			fl.g0.numElementsPacked = r();
			fl.g0.data = reinterpret_cast<unsigned char*>(malloc(fl.g0.numElementsPacked));
			fread(fl.g0.data, 1, fl.g0.numElementsPacked, fp);
			
			fl.g1.res = r();
			fl.g1.numElements = r();
			fl.g1.numElementsPacked = r();
			fl.g1.data = reinterpret_cast<unsigned char*>(malloc(fl.g1.numElementsPacked));
			fread(fl.g1.data, 1, fl.g1.numElementsPacked, fp);
		}
		
		model.mlp.resize(model.header.numLinearLayers);
		for (NTCLinearLayer& l : model.mlp)
		{
			l.inFeatures = r();
			l.outFeatures = r();
			
			l.weights.resize(l.inFeatures * l.outFeatures);
			fread(l.weights.data(), sizeof(std::uint16_t), l.weights.size(), fp);
			
			l.biases.resize(l.outFeatures);
			fread(l.biases.data(), sizeof(std::uint16_t), l.biases.size(), fp);
		}

		//Add to cache
		m_filepathToNTCModelCache[_filepath] = UniquePtr<NTCModel>(NK_NEW(NTCModel, model));
		return m_filepathToNTCModelCache[_filepath].get();
	}


	
	void NTCLoader::FreeMaterial(NTCModel* _modelData)
	{
		if (!_modelData) { throw std::runtime_error("NTCLoader::FreeMaterial() - Attempted to free uninitialised / already freed material."); }
		
		for (std::unordered_map<std::string, UniquePtr<NTCModel>>::iterator it{ m_filepathToNTCModelCache.begin() }; it != m_filepathToNTCModelCache.end(); ++it)
		{
			if (it->second.get() == _modelData)
			{
				for (const NTCFeatureLevel& fl : _modelData->featureLevels)
				{
					free(fl.g0.data);
					free(fl.g1.data);
				}
				m_filepathToNTCModelCache.erase(it->first);
				return;
			}
		}
	}

	
	
	void NTCLoader::ClearCache()
	{
		for (std::unordered_map<std::string, UniquePtr<NTCModel>>::iterator it{ m_filepathToNTCModelCache.begin() }; it != m_filepathToNTCModelCache.end(); ++it)
		{
			if (it->second)
			{
				for (const NTCFeatureLevel& fl : it->second->featureLevels)
				{
					free(fl.g0.data);
					free(fl.g1.data);
				}
			}
		}
		m_filepathToNTCModelCache.clear();
	}
	
}
