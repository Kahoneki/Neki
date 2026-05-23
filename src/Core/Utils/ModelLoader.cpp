#include "ModelLoader.h"

#include "ImageLoader.h"
#include "TextureCompressor.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cereal/archives/binary.hpp>

#include "imgui.h"
#ifdef max
	#undef max
#endif
#ifdef _WIN32
	#define POPEN _popen
	#define PCLOSE _pclose
#else
	#define POPEN popen
	#define PCLOSE pclose
#endif


namespace NK
{

	std::map<std::pair<std::string, std::uint64_t>, UniquePtr<CPUMeshData>> ModelLoader::m_filepathToMeshDataCache;



	const CPUMeshData* ModelLoader::LoadMesh(const std::string& _filepath, std::uint64_t _offset)
	{
		if (!std::filesystem::path(_filepath).has_extension() || std::filesystem::path(_filepath).extension() != ".nkmeshdata")
		{
			throw std::invalid_argument("ModelLoader::LoadMesh() - _filepath does not end in .nkmeshdata as required. _filepath = " + _filepath);
		}
		
		const std::map<std::pair<std::string, std::uint64_t>, UniquePtr<CPUMeshData>>::iterator it{ m_filepathToMeshDataCache.find(std::make_pair(_filepath, _offset)) };
		if (it != m_filepathToMeshDataCache.end())
		{
			//Model has already been loaded, pull from cache
			return it->second.get();
		}
		
		CPUMeshData cpuMeshData{};
		std::ifstream fs(_filepath, std::ios::binary);
		if (!fs)
		{
			throw std::invalid_argument("ModelLoader::LoadMesh() - failed to open model at _filepath (" + _filepath + ")");
		}
		
		fs.seekg(_offset);
		cereal::BinaryInputArchive meshArch(fs);
		meshArch(cpuMeshData);
		m_filepathToMeshDataCache[std::make_pair(_filepath, _offset)] = UniquePtr<CPUMeshData>(NK_NEW(CPUMeshData, cpuMeshData));
		return m_filepathToMeshDataCache[std::make_pair(_filepath, _offset)].get();
	}

	

	void ModelLoader::UnloadMesh(const std::string& _filepath, std::uint64_t _offset)
	{
		if (!m_filepathToMeshDataCache.contains(std::make_pair(_filepath, _offset)))
		{
			throw std::invalid_argument("ModelLoader::UnloadModel() - _filepath-offset pair not in cache");
		}
		m_filepathToMeshDataCache.erase(std::make_pair(_filepath, _offset));
	}

	
	
	std::variant<CPUMaterial, CPUMaterialNTC> ModelLoader::GetMaterialHeader(const std::string& _filepath)
	{
		if (!std::filesystem::path(_filepath).has_extension() || (std::filesystem::path(_filepath).extension() != ".nkmaterial" && std::filesystem::path(_filepath).extension() != ".nkmaterialntc"))
		{
			throw std::invalid_argument("ModelLoader::GetMaterialHeader() - _filepath does not end in .nkmaterial as required. _filepath = " + _filepath);
		}
		
		std::ifstream fs(_filepath, std::ios::binary);
		if (!fs)
		{
			throw std::invalid_argument("ModelLoader::GetMaterialHeader() - failed to open model at _filepath (" + _filepath + ")");
		}
		cereal::BinaryInputArchive archive(fs);

		DiskMaterial header{};
		archive(header);
		if (header.magic != std::string("NKMATERIAL") && header.magic != std::string("NKMATERIALNTC"))
		{
			throw std::runtime_error("ModelLoader::GetMaterialHeader() - deserialised material header's magic string was not the expected \"NKMATERIAL\" - header.magic = " + header.magic);
		}
		
		if (header.isNTC)
		{
			CPUMaterialNTC cpuHeaderNTC;
			cpuHeaderNTC.name = header.name;
			cpuHeaderNTC.pipeline = header.pipeline;
			cpuHeaderNTC.materialDataFilepath = header.ntcMaterialDataFilepath;
			cpuHeaderNTC.numChannels = header.numChannels;
			cpuHeaderNTC.materialPropertyChannelLookup = header.materialPropertyChannelLookup;
			cpuHeaderNTC.shaderMaterialData = header.ntcShaderMaterialData;
			return cpuHeaderNTC;
		}
		else
		{
			CPUMaterial cpuHeader;
			cpuHeader.name = header.name;
			cpuHeader.pipeline = header.pipeline;
			cpuHeader.shaderMaterialData = header.shaderMaterialData;
			cpuHeader.allTextures = header.allTextures;
			return cpuHeader;
		}
	}


	
	VertexInputDesc ModelLoader::GetModelVertexInputDescription()
	{
		std::vector<VertexAttributeDesc> vertexAttributes;
		
		//Position attribute
		VertexAttributeDesc posAttribute{};
		posAttribute.attribute = SHADER_ATTRIBUTE::POSITION;
		posAttribute.binding = 0;
		posAttribute.format = DATA_FORMAT::R32G32B32_SFLOAT;
		posAttribute.offset = offsetof(Vertex, position);
		vertexAttributes.push_back(posAttribute);

		//Normal attribute
		VertexAttributeDesc normAttribute{};
		normAttribute.attribute = SHADER_ATTRIBUTE::NORMAL;
		normAttribute.binding = 0;
		normAttribute.format = DATA_FORMAT::R32G32B32_SFLOAT;
		normAttribute.offset = offsetof(Vertex, normal);
		vertexAttributes.push_back(normAttribute);

		//Texcoord attribute
		VertexAttributeDesc uvAttribute{};
		uvAttribute.attribute = SHADER_ATTRIBUTE::TEXCOORD_0;
		uvAttribute.binding = 0;
		uvAttribute.format = DATA_FORMAT::R32G32_SFLOAT;
		uvAttribute.offset = offsetof(Vertex, texCoord);
		vertexAttributes.push_back(uvAttribute);

		//Tangent attribute
		VertexAttributeDesc tanAttribute{};
		tanAttribute.attribute = SHADER_ATTRIBUTE::TANGENT;
		tanAttribute.binding = 0;
		tanAttribute.format = DATA_FORMAT::R32G32B32A32_SFLOAT;
		tanAttribute.offset = offsetof(Vertex, tangent);
		vertexAttributes.push_back(tanAttribute);

		//Vertex buffer binding
		std::vector<VertexBufferBindingDesc> bufferBindings;
		VertexBufferBindingDesc bufferBinding{};
		bufferBinding.binding = 0;
		bufferBinding.inputRate = VERTEX_INPUT_RATE::VERTEX;
		bufferBinding.stride = sizeof(Vertex);
		bufferBindings.push_back(bufferBinding);

		//Vertex input description
		VertexInputDesc vertexInputDesc{};
		vertexInputDesc.attributeDescriptions = vertexAttributes;
		vertexInputDesc.bufferBindingDescriptions = bufferBindings;

		return vertexInputDesc;
	}

	
	
	void ModelLoader::ClearCache()
	{
		m_filepathToMeshDataCache.clear();
		ImageLoader::ClearCache();
		TextureCompressor::ClearCache();
	}


	
	void ModelLoader::SerialiseNKModel(const std::string& _inputFilepath, const std::string& _outputFilepath, bool _flipFaceWinding, bool _flipTextures)
	{
		//Create output filepaths if it doesn't exist
		std::filesystem::path outputPath{ _outputFilepath };
		const std::filesystem::path outputDir{ outputPath.parent_path() };
		if (!outputDir.empty())
		{
			std::filesystem::create_directories(outputDir);
		}
		
		DiskModel diskModel;
		diskModel.magic = "NKMODEL";
		diskModel.version = 0;
		diskModel.cpuModel.meshDataFilepath = outputPath.replace_extension(".nkmeshdata");
		
		std::pair<std::vector<CPUMeshData>, std::vector<CPUMaterial>> data{ LoadNonNKModelData(_inputFilepath, _flipFaceWinding, _flipTextures, outputDir) };
		std::ofstream meshStream(diskModel.cpuModel.meshDataFilepath, std::ios::binary);
		if (!meshStream)
		{
			throw std::runtime_error("ModelLoader::SerialiseNKModel() - failed to create mesh data file. Filepath = " + diskModel.cpuModel.meshDataFilepath);
		}
		{
			cereal::BinaryOutputArchive meshArch(meshStream);
			std::string magic{ "NKMESHDATA" };
			meshArch(magic);
			cereal::size_type numMeshes{ data.first.size() };
			meshArch(numMeshes);
			diskModel.cpuModel.meshDataLoadInfo.resize(numMeshes);
			
			//Calculate centre and halfExtents of model
			glm::vec3 minAABBModel(std::numeric_limits<float>::max());
			glm::vec3 maxAABBModel(std::numeric_limits<float>::lowest());
			
			for (std::size_t i{ 0 }; i < numMeshes; ++i)
			{
				//Calculate halfExtents of mesh
				glm::vec3 minAABB(std::numeric_limits<float>::max());
				glm::vec3 maxAABB(std::numeric_limits<float>::lowest());
				for (const Vertex& vertex : data.first[i].vertices)
				{
					minAABB.x = std::min(minAABB.x, vertex.position.x);
					minAABB.y = std::min(minAABB.y, vertex.position.y);
					minAABB.z = std::min(minAABB.z, vertex.position.z);

					maxAABB.x = std::max(maxAABB.x, vertex.position.x);
					maxAABB.y = std::max(maxAABB.y, vertex.position.y);
					maxAABB.z = std::max(maxAABB.z, vertex.position.z);
					
					//Also update model's half extents
					minAABBModel.x = std::min(minAABBModel.x, vertex.position.x);
					minAABBModel.y = std::min(minAABBModel.y, vertex.position.y);
					minAABBModel.z = std::min(minAABBModel.z, vertex.position.z);

					maxAABBModel.x = std::max(maxAABBModel.x, vertex.position.x);
					maxAABBModel.y = std::max(maxAABBModel.y, vertex.position.y);
					maxAABBModel.z = std::max(maxAABBModel.z, vertex.position.z);
				}
				diskModel.cpuModel.meshDataLoadInfo[i].centre = (minAABB + maxAABB) * 0.5f;
				diskModel.cpuModel.meshDataLoadInfo[i].halfExtents = (maxAABB - minAABB) * 0.5f;
				
				diskModel.cpuModel.meshDataLoadInfo[i].meshOffset = meshStream.tellp();
				meshArch(data.first[i]);
			}
			
			diskModel.cpuModel.halfExtents = (maxAABBModel - minAABBModel) * 0.5f;
		}
		
		std::ofstream modelStream(_outputFilepath, std::ios::binary);
		if (!modelStream)
		{
			throw std::runtime_error("ModelLoader::SerialiseNKModel() - failed to failed to create model data file. Filepath = " + _outputFilepath);
		}
		{
			cereal::BinaryOutputArchive modelArch(modelStream);
			modelArch(diskModel);
		}
		
		for (std::size_t i{ 0 }; i < data.second.size(); ++i)
		{
			DiskMaterial diskMaterial{};
			diskMaterial.magic = "NKMATERIAL";
			diskMaterial.version = 0;
			diskMaterial.name = data.second[i].name;
			diskMaterial.pipeline = data.second[i].pipeline;
			diskMaterial.isNTC = false; //default
			diskMaterial.shaderMaterialData = data.second[i].shaderMaterialData;
			diskMaterial.allTextures = data.second[i].allTextures;
			
			std::string materialOutputPath{ outputDir.string() + "/" + diskMaterial.name + std::string(".nkmaterial") };
			std::ofstream materialStream(materialOutputPath, std::ios::binary);
			if (!materialStream)
			{
				throw std::runtime_error("ModelLoader::SerialiseNKModel() - failed to create material file. Filepath = " + materialOutputPath);
			}
			{
				cereal::BinaryOutputArchive materialArch(materialStream);
				materialArch(diskMaterial);
			}
		}
	}

	
	
	void ModelLoader::SerialiseNKModelNTC(const std::string& _inputFilepath, const std::string& _outputFilepath, bool _flipFaceWinding, bool _flipTextures, const NeuralTrainingParameters& _neuralTrainingParameters)
	{
		//Validate neural training parameters
		if (_neuralTrainingParameters.quality < 0 || _neuralTrainingParameters.quality > 3)
		{
			throw std::invalid_argument("SerialiseNKModelNTC() - provided _neuralTrainingParameters.quality is not in valid range [0,3] - quality = " + std::to_string(_neuralTrainingParameters.quality));
		}
		if (_neuralTrainingParameters.hiddenNeurons <= 0)
		{
			throw std::invalid_argument("SerialiseNKModelNTC() - provided _neuralTrainingParameters.hiddenNeurons must be > 0 - hiddenNeurons = " + std::to_string(_neuralTrainingParameters.hiddenNeurons));
		}
		if (_neuralTrainingParameters.epochs <= 0)
		{
			throw std::invalid_argument("SerialiseNKModelNTC() - provided _neuralTrainingParameters.epochs must be > 0 - epochs = " + std::to_string(_neuralTrainingParameters.epochs));
		}
		
		
		//Create output filepath if it doesn't exist
		std::filesystem::path outputPath{ _outputFilepath };
		const std::filesystem::path outputDir{ outputPath.parent_path() };
		if (!outputDir.empty())
		{
			std::filesystem::create_directories(outputDir);
		}
		
		DiskModel diskModel;
		diskModel.magic = "NKMODEL";
		diskModel.version = 0;
		diskModel.cpuModel.meshDataFilepath = outputPath.replace_extension(".nkmeshdata");
		
		std::pair<std::vector<CPUMeshData>, std::vector<std::variant<CPUMaterial, CPUMaterialNTC>>> data{ LoadNonNKModelDataNTC(_inputFilepath, _flipFaceWinding, _flipTextures, outputDir, _neuralTrainingParameters) };
		std::ofstream meshStream(diskModel.cpuModel.meshDataFilepath, std::ios::binary);
		if (!meshStream)
		{
			throw std::runtime_error("ModelLoader::SerialiseNKModelNTC() - failed to create mesh data file. Filepath = " + diskModel.cpuModel.meshDataFilepath);
		}
		{
			cereal::BinaryOutputArchive meshArch(meshStream);
			std::string magic{ "NKMESHDATA" };
			meshArch(magic);
			cereal::size_type numMeshes{ data.first.size() };
			meshArch(numMeshes);
			diskModel.cpuModel.meshDataLoadInfo.resize(numMeshes);
			
			//Calculate centre and halfExtents of model
			glm::vec3 minAABBModel(std::numeric_limits<float>::max());
			glm::vec3 maxAABBModel(std::numeric_limits<float>::lowest());
			
			for (std::size_t i{ 0 }; i < numMeshes; ++i)
			{
				//Calculate halfExtents of mesh
				glm::vec3 minAABB(std::numeric_limits<float>::max());
				glm::vec3 maxAABB(std::numeric_limits<float>::lowest());
				for (const Vertex& vertex : data.first[i].vertices)
				{
					minAABB.x = std::min(minAABB.x, vertex.position.x);
					minAABB.y = std::min(minAABB.y, vertex.position.y);
					minAABB.z = std::min(minAABB.z, vertex.position.z);

					maxAABB.x = std::max(maxAABB.x, vertex.position.x);
					maxAABB.y = std::max(maxAABB.y, vertex.position.y);
					maxAABB.z = std::max(maxAABB.z, vertex.position.z);
					
					//Also update model's half extents
					minAABBModel.x = std::min(minAABBModel.x, vertex.position.x);
					minAABBModel.y = std::min(minAABBModel.y, vertex.position.y);
					minAABBModel.z = std::min(minAABBModel.z, vertex.position.z);

					maxAABBModel.x = std::max(maxAABBModel.x, vertex.position.x);
					maxAABBModel.y = std::max(maxAABBModel.y, vertex.position.y);
					maxAABBModel.z = std::max(maxAABBModel.z, vertex.position.z);
				}
				diskModel.cpuModel.meshDataLoadInfo[i].centre = (minAABB + maxAABB) * 0.5f;
				diskModel.cpuModel.meshDataLoadInfo[i].halfExtents = (maxAABB - minAABB) * 0.5f;
				
				diskModel.cpuModel.meshDataLoadInfo[i].meshOffset = meshStream.tellp();
				meshArch(data.first[i]);
			}
			
			diskModel.cpuModel.halfExtents = (maxAABBModel - minAABBModel) * 0.5f;
		}
		
		std::ofstream modelStream(_outputFilepath, std::ios::binary);
		if (!modelStream)
		{
			throw std::runtime_error("ModelLoader::SerialiseNKModelNTC() - failed to failed to create model data file. Filepath = " + _outputFilepath);
		}
		{
			cereal::BinaryOutputArchive modelArch(modelStream);
			modelArch(diskModel);
		}
		
		for (std::size_t i{ 0 }; i < data.second.size(); ++i)
		{
			DiskMaterial diskMaterial{};
			diskMaterial.version = 0;
			
			//Check which variant was returned by the loader
			if (std::holds_alternative<CPUMaterialNTC>(data.second[i]))
			{
				const CPUMaterialNTC& ntcMat{ std::get<CPUMaterialNTC>(data.second[i]) };
				diskMaterial.magic = "NKMATERIALNTC";
				diskMaterial.name = ntcMat.name;
				diskMaterial.pipeline = ntcMat.pipeline;
				diskMaterial.isNTC = true;
				diskMaterial.ntcMaterialDataFilepath = ntcMat.materialDataFilepath;
				diskMaterial.numChannels = ntcMat.numChannels;
				diskMaterial.materialPropertyChannelLookup = ntcMat.materialPropertyChannelLookup;
				diskMaterial.ntcShaderMaterialData = ntcMat.shaderMaterialData;
			}
			else
			{
				const CPUMaterial& stdMat{ std::get<CPUMaterial>(data.second[i]) };
				diskMaterial.magic = "NKMATERIAL";
				diskMaterial.name = stdMat.name;
				diskMaterial.pipeline = stdMat.pipeline;
				diskMaterial.isNTC = false;
				diskMaterial.shaderMaterialData = stdMat.shaderMaterialData;
				diskMaterial.allTextures = stdMat.allTextures;
			}
			
			const std::string ext{ (diskMaterial.isNTC ? ".nkmaterialntc" : ".nkmaterial") };
			std::string materialOutputPath{ outputDir.string() + "/" + diskMaterial.name + ext };
			std::ofstream materialStream(materialOutputPath, std::ios::binary);
			if (!materialStream)
			{
				throw std::runtime_error("ModelLoader::SerialiseNKModelNTC() - failed to create material file. Filepath = " + materialOutputPath);
			}
			{
				cereal::BinaryOutputArchive materialArch(materialStream);
				materialArch(diskMaterial);
			}
		}
	}


	CPUModel ModelLoader::GetNKModelHeader(const std::string& _filepath)
	{
		std::ifstream fs(_filepath, std::ios::binary);
		if (!fs)
		{
			throw std::invalid_argument("ModelLoader::GetNKModelHeader() - failed to open model at _filepath (" + _filepath + ")");
		}
		cereal::BinaryInputArchive archive(fs);

		DiskModel header{};
		archive(header);
		if (header.magic != std::string("NKMODEL"))
		{
			throw std::runtime_error("ModelLoader::GetNKModelHeader() - deserialised model header's magic string was not the expected \"NKMODEL\" - header.magic = " + header.magic);
		}
		
		return header.cpuModel;
	}



	std::pair<std::vector<CPUMeshData>, std::vector<CPUMaterial>> ModelLoader::LoadNonNKModelData(const std::string& _filepath, bool _flipFaceWinding, bool _flipTextures, const std::string& _serialisedModelOutputDirectory)
	{
		Assimp::Importer importer{};
		const aiScene* scene{ importer.ReadFile(_filepath,
		                                        aiProcess_Triangulate |			//Ensure model is composed of triangles
		                                        aiProcess_GenSmoothNormals |	//Generate smooth normals if they don't exist
		                                        /*aiProcess_FlipUVs |*/			//Flip UVs to match Vulkan's top left texcoord system
		                                        aiProcess_CalcTangentSpace |	//Calculate tangents and bitangents (required for TBN in normal mapping)
		                                        aiProcess_MakeLeftHanded |
		                                        aiProcess_JoinIdenticalVertices |
		                                        aiProcess_PreTransformVertices |
		                                        (_flipFaceWinding ? aiProcess_FlipWindingOrder : 0)
		                                       ) };

		//Ensure scene was loaded correctly
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			throw std::runtime_error("ModelLoader::LoadModel() - Failed to load model (" + _filepath + ") - " + std::string(importer.GetErrorString()));
		}

		
		//Load mesh data
		std::vector<CPUMeshData> cpuMeshData;
		const std::string& materialDirectory{ _serialisedModelOutputDirectory };
		ProcessNode(scene->mRootNode, scene, &cpuMeshData, materialDirectory);
		
		
		//Calculate model extents
		glm::vec3 minAABB(std::numeric_limits<float>::max());
		glm::vec3 maxAABB(std::numeric_limits<float>::lowest());
		for (const CPUMeshData& mesh : cpuMeshData)
		{
			for (const Vertex& vertex : mesh.vertices)
			{
				minAABB.x = std::min(minAABB.x, vertex.position.x);
				minAABB.y = std::min(minAABB.y, vertex.position.y);
				minAABB.z = std::min(minAABB.z, vertex.position.z);

				maxAABB.x = std::max(maxAABB.x, vertex.position.x);
				maxAABB.y = std::max(maxAABB.y, vertex.position.y);
				maxAABB.z = std::max(maxAABB.z, vertex.position.z);
			}
		}
		const glm::vec3 extentsCentre{ (minAABB + maxAABB) * 0.5f };

		//Center the model so its local origin (0,0,0) is at centre of extents
		for (CPUMeshData& mesh : cpuMeshData)
		{
			for (Vertex& vertex : mesh.vertices)
			{
				vertex.position -= extentsCentre;
			}
		}
		
		
		//Load materials
		std::vector<CPUMaterial> materials;
		materials.resize(scene->mNumMaterials);
		for (std::size_t i{ 0 }; i < scene->mNumMaterials; ++i)
		{
			aiMaterial* assimpMaterial{ scene->mMaterials[i] };
			std::string matName{ assimpMaterial->GetName().C_Str() };
			if (matName.empty()) { matName = "Material"; }
			matName += "_" + std::to_string(i);
			std::ranges::replace_if(matName,[](const char c) { return c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|'; }, '_');
			materials[i].name = matName;
			
			auto load{ [&](const MODEL_TEXTURE_TYPE _dst, const aiTextureType _src)
			{
				materials[i].allTextures[std::to_underlying(_dst)] = GetMaterialTextureDataForSerialisation(assimpMaterial, static_cast<aiTextureTypeOverload>(_src), _dst);
				if (!materials[i].allTextures.at(std::to_underlying(_dst)).first.empty())
				{
					//Texture was added, compress to ktx2
					std::string& filepath{ materials[i].allTextures.at(std::to_underlying(_dst)).first };
					const std::string newFilepath{ (_serialisedModelOutputDirectory / std::filesystem::path(filepath)).lexically_normal().replace_extension(".ktx2").string() };
					TextureCompressor::KTXCompress(std::filesystem::path(_filepath).parent_path() / filepath, materials[i].allTextures.at(std::to_underlying(_dst)).second, _flipTextures, newFilepath);
					filepath = std::filesystem::path(newFilepath).string(); //filepath is a reference so this is modifying the lookup entry to point to the new ktx2 texture
				}
			}};
			
			load(MODEL_TEXTURE_TYPE::DIFFUSE          , aiTextureType_DIFFUSE);
			load(MODEL_TEXTURE_TYPE::SPECULAR         , aiTextureType_SPECULAR);
			load(MODEL_TEXTURE_TYPE::AMBIENT          , aiTextureType_AMBIENT);
			load(MODEL_TEXTURE_TYPE::EMISSIVE         , aiTextureType_EMISSIVE);
			load(MODEL_TEXTURE_TYPE::HEIGHT           , aiTextureType_HEIGHT);
			load(MODEL_TEXTURE_TYPE::NORMAL           , aiTextureType_NORMAL_CAMERA); //Preferred type
			load(MODEL_TEXTURE_TYPE::SHININESS        , aiTextureType_SHININESS);
			load(MODEL_TEXTURE_TYPE::OPACITY          , aiTextureType_OPACITY);
			load(MODEL_TEXTURE_TYPE::DISPLACEMENT     , aiTextureType_DISPLACEMENT);
			load(MODEL_TEXTURE_TYPE::LIGHTMAP         , aiTextureType_LIGHTMAP);
			load(MODEL_TEXTURE_TYPE::REFLECTION       , aiTextureType_REFLECTION);
			load(MODEL_TEXTURE_TYPE::BASE_COLOUR      , aiTextureType_BASE_COLOR);
			load(MODEL_TEXTURE_TYPE::METALNESS        , aiTextureType_METALNESS);
			load(MODEL_TEXTURE_TYPE::ROUGHNESS        , aiTextureType_DIFFUSE_ROUGHNESS);
			load(MODEL_TEXTURE_TYPE::EMISSION_COLOUR  , aiTextureType_EMISSION_COLOR);
			load(MODEL_TEXTURE_TYPE::AMBIENT_OCCLUSION, aiTextureType_AMBIENT_OCCLUSION);
			
			//If NORMAL_CAMERA wasn't available, fall back to NORMAL
			if (materials[i].allTextures[std::to_underlying(MODEL_TEXTURE_TYPE::NORMAL)].first.empty())
			{
				load(MODEL_TEXTURE_TYPE::NORMAL, aiTextureType_NORMALS);
			}
			
			//Determine lighting model
			auto hasTex{ [&](const MODEL_TEXTURE_TYPE _tex) { return !materials[i].allTextures[std::to_underlying(_tex)].first.empty(); } };
			const bool isPBR{	hasTex(MODEL_TEXTURE_TYPE::METALNESS)			||
								hasTex(MODEL_TEXTURE_TYPE::ROUGHNESS)			||
								hasTex(MODEL_TEXTURE_TYPE::BASE_COLOUR)			||
								hasTex(MODEL_TEXTURE_TYPE::AMBIENT_OCCLUSION)	||
								hasTex(MODEL_TEXTURE_TYPE::EMISSION_COLOUR) };
			materials[i].pipeline = (isPBR ? LIGHTING_MODEL::PHYSICALLY_BASED : LIGHTING_MODEL::BLINN_PHONG);
			
			//Populate shader material data
			switch (materials[i].pipeline)
			{
			case LIGHTING_MODEL::BLINN_PHONG:
			{
				materials[i].shaderMaterialData = BlinnPhongMaterial{};
				BlinnPhongMaterial& material{ std::get<BlinnPhongMaterial>(materials[i].shaderMaterialData) };
				
				//Convenient workaround - see explanation in CPUMaterial declaration (ModelLoader.h)
				material.diffuseIdx			= static_cast<int>(MODEL_TEXTURE_TYPE::DIFFUSE);
				material.specularIdx		= static_cast<int>(MODEL_TEXTURE_TYPE::SPECULAR);
				material.ambientIdx			= hasTex(MODEL_TEXTURE_TYPE::AMBIENT) ? static_cast<int>(MODEL_TEXTURE_TYPE::AMBIENT) : hasTex(MODEL_TEXTURE_TYPE::LIGHTMAP) ? static_cast<int>(MODEL_TEXTURE_TYPE::LIGHTMAP) : static_cast<int>(MODEL_TEXTURE_TYPE::AMBIENT_OCCLUSION);
				material.emissiveIdx		= hasTex(MODEL_TEXTURE_TYPE::EMISSION_COLOUR) ? static_cast<int>(MODEL_TEXTURE_TYPE::EMISSION_COLOUR) : static_cast<int>(MODEL_TEXTURE_TYPE::EMISSIVE);
				material.normalIdx			= static_cast<int>(MODEL_TEXTURE_TYPE::NORMAL);
				material.shininessIdx		= static_cast<int>(MODEL_TEXTURE_TYPE::SHININESS);
				material.opacityIdx			= static_cast<int>(MODEL_TEXTURE_TYPE::OPACITY);
				material.heightIdx			= static_cast<int>(MODEL_TEXTURE_TYPE::HEIGHT);
				material.displacementIdx	= static_cast<int>(MODEL_TEXTURE_TYPE::DISPLACEMENT);
				material.lightmapIdx		= static_cast<int>(MODEL_TEXTURE_TYPE::LIGHTMAP);
				material.reflectionIdx		= static_cast<int>(MODEL_TEXTURE_TYPE::REFLECTION);
				
				material.hasDiffuse			= hasTex(MODEL_TEXTURE_TYPE::DIFFUSE) ? 1 : 0;
				material.hasSpecular		= hasTex(MODEL_TEXTURE_TYPE::SPECULAR) ? 1 : 0;
				material.hasAmbient			= hasTex(MODEL_TEXTURE_TYPE::AMBIENT) || hasTex(MODEL_TEXTURE_TYPE::LIGHTMAP) || hasTex(MODEL_TEXTURE_TYPE::AMBIENT_OCCLUSION) ? 1 : 0;
				material.hasEmissive		= hasTex(MODEL_TEXTURE_TYPE::EMISSION_COLOUR) || hasTex(MODEL_TEXTURE_TYPE::EMISSIVE) ? 1 : 0;
				material.hasNormal			= hasTex(MODEL_TEXTURE_TYPE::NORMAL) ? 1 : 0;
				material.hasShininess		= hasTex(MODEL_TEXTURE_TYPE::SHININESS) ? 1 : 0;
				material.hasOpacity			= hasTex(MODEL_TEXTURE_TYPE::OPACITY) ? 1 : 0;
				material.hasHeight			= hasTex(MODEL_TEXTURE_TYPE::HEIGHT) ? 1 : 0;
				material.hasDisplacement	= hasTex(MODEL_TEXTURE_TYPE::DISPLACEMENT) ? 1 : 0;
				material.hasLightmap		= hasTex(MODEL_TEXTURE_TYPE::LIGHTMAP) ? 1 : 0;
				material.hasReflection		= hasTex(MODEL_TEXTURE_TYPE::REFLECTION) ? 1 : 0;
				
				break;
			}

			case LIGHTING_MODEL::PHYSICALLY_BASED:
			{
				materials[i].shaderMaterialData = PBRMaterial{};
				PBRMaterial& material{ std::get<PBRMaterial>(materials[i].shaderMaterialData) };

				//Convenient workaround - see explanation in CPUMaterial declaration (ModelLoader.h)
				material.baseColourIdx		= hasTex(MODEL_TEXTURE_TYPE::BASE_COLOUR) ? static_cast<int>(MODEL_TEXTURE_TYPE::BASE_COLOUR) : static_cast<int>(MODEL_TEXTURE_TYPE::DIFFUSE);
				material.metalnessIdx		= static_cast<int>(MODEL_TEXTURE_TYPE::METALNESS);
				material.roughnessIdx		= static_cast<int>(MODEL_TEXTURE_TYPE::ROUGHNESS);
				material.specularIdx		= static_cast<int>(MODEL_TEXTURE_TYPE::SPECULAR);
				material.shininessIdx		= static_cast<int>(MODEL_TEXTURE_TYPE::SHININESS);
				material.normalIdx			= static_cast<int>(MODEL_TEXTURE_TYPE::NORMAL);
				material.aoIdx				= hasTex(MODEL_TEXTURE_TYPE::AMBIENT_OCCLUSION) ? static_cast<int>(MODEL_TEXTURE_TYPE::AMBIENT_OCCLUSION) : hasTex(MODEL_TEXTURE_TYPE::LIGHTMAP) ? static_cast<int>(MODEL_TEXTURE_TYPE::LIGHTMAP) : static_cast<int>(MODEL_TEXTURE_TYPE::AMBIENT);
				material.emissiveIdx		= hasTex(MODEL_TEXTURE_TYPE::EMISSION_COLOUR) ? static_cast<int>(MODEL_TEXTURE_TYPE::EMISSION_COLOUR) : static_cast<int>(MODEL_TEXTURE_TYPE::EMISSIVE);
				material.opacityIdx			= static_cast<int>(MODEL_TEXTURE_TYPE::OPACITY);
				material.heightIdx			= static_cast<int>(MODEL_TEXTURE_TYPE::HEIGHT);
				material.displacementIdx	= static_cast<int>(MODEL_TEXTURE_TYPE::DISPLACEMENT);
				material.reflectionIdx		= static_cast<int>(MODEL_TEXTURE_TYPE::REFLECTION);
				
				material.hasBaseColour		= hasTex(MODEL_TEXTURE_TYPE::BASE_COLOUR) || hasTex(MODEL_TEXTURE_TYPE::DIFFUSE) ? 1 : 0;
				material.hasMetalness		= hasTex(MODEL_TEXTURE_TYPE::METALNESS) ? 1 : 0;
				material.hasRoughness		= hasTex(MODEL_TEXTURE_TYPE::ROUGHNESS) ? 1 : 0;
				material.hasSpecular		= hasTex(MODEL_TEXTURE_TYPE::SPECULAR) ? 1 : 0;
				material.hasShininess		= hasTex(MODEL_TEXTURE_TYPE::SHININESS) ? 1 : 0;
				material.hasNormal			= hasTex(MODEL_TEXTURE_TYPE::NORMAL) ? 1 : 0;
				material.hasAO				= hasTex(MODEL_TEXTURE_TYPE::AMBIENT_OCCLUSION) || hasTex(MODEL_TEXTURE_TYPE::LIGHTMAP) || hasTex(MODEL_TEXTURE_TYPE::AMBIENT) ? 1 : 0;
				material.hasEmissive		= hasTex(MODEL_TEXTURE_TYPE::EMISSION_COLOUR)	|| hasTex(MODEL_TEXTURE_TYPE::EMISSIVE) ? 1 : 0;
				material.hasOpacity			= hasTex(MODEL_TEXTURE_TYPE::OPACITY) ? 1 : 0;
				material.hasHeight			= hasTex(MODEL_TEXTURE_TYPE::HEIGHT) ? 1 : 0;
				material.hasDisplacement	= hasTex(MODEL_TEXTURE_TYPE::DISPLACEMENT) ? 1 : 0;
				material.hasReflection		= hasTex(MODEL_TEXTURE_TYPE::REFLECTION) ? 1 : 0;
				
				break;
			}
			}
		}
		
		return std::make_pair<std::vector<CPUMeshData>, std::vector<CPUMaterial>>(std::move(cpuMeshData), std::move(materials));
	}

	
	
	std::pair<std::vector<CPUMeshData>, std::vector<std::variant<CPUMaterial, CPUMaterialNTC>>> ModelLoader::LoadNonNKModelDataNTC(const std::string& _filepath, bool _flipFaceWinding, bool _flipTextures, const std::string& _serialisedModelOutputDirectory, const NeuralTrainingParameters& _neuralTrainingParameters)
	{
		Assimp::Importer importer{};
		const aiScene* scene{ importer.ReadFile(_filepath,
		                                        aiProcess_Triangulate |			//Ensure model is composed of triangles
		                                        aiProcess_GenSmoothNormals |	//Generate smooth normals if they don't exist
		                                        /*aiProcess_FlipUVs |*/			//Flip UVs to match Vulkan's top left texcoord system
		                                        aiProcess_CalcTangentSpace |	//Calculate tangents and bitangents (required for TBN in normal mapping)
		                                        aiProcess_MakeLeftHanded |
		                                        aiProcess_JoinIdenticalVertices |
		                                        (_flipFaceWinding ? aiProcess_FlipWindingOrder : 0)
		                                       ) };

		//Ensure scene was loaded correctly
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			throw std::runtime_error("ModelLoader::LoadModel() - Failed to load model (" + _filepath + ") - " + std::string(importer.GetErrorString()));
		}

		
		//Load mesh data
		std::vector<CPUMeshData> cpuMeshData;
		const std::string& materialDirectory{ _serialisedModelOutputDirectory };
		ProcessNode(scene->mRootNode, scene, &cpuMeshData, materialDirectory);
		
		
		//Calculate model extents
		glm::vec3 minAABB(std::numeric_limits<float>::max());
		glm::vec3 maxAABB(std::numeric_limits<float>::lowest());
		for (const CPUMeshData& mesh : cpuMeshData)
		{
			for (const Vertex& vertex : mesh.vertices)
			{
				minAABB.x = std::min(minAABB.x, vertex.position.x);
				minAABB.y = std::min(minAABB.y, vertex.position.y);
				minAABB.z = std::min(minAABB.z, vertex.position.z);

				maxAABB.x = std::max(maxAABB.x, vertex.position.x);
				maxAABB.y = std::max(maxAABB.y, vertex.position.y);
				maxAABB.z = std::max(maxAABB.z, vertex.position.z);
			}
		}
		const glm::vec3 extentsCentre{ (minAABB + maxAABB) * 0.5f };

		//Center the model so its local origin (0,0,0) is at centre of extents
		for (CPUMeshData& mesh : cpuMeshData)
		{
			for (Vertex& vertex : mesh.vertices)
			{
				vertex.position -= extentsCentre;
			}
		}
		
		
		//Load materials
		std::vector<std::variant<CPUMaterial, CPUMaterialNTC>> materials;
		materials.resize(scene->mNumMaterials);
		for (std::size_t i{ 0 }; i < scene->mNumMaterials; ++i)
		{
			aiMaterial* assimpMaterial{ scene->mMaterials[i] };
			std::string matName{ assimpMaterial->GetName().C_Str() };
			if (matName.empty()) { matName = "Material"; }
			matName += "_" + std::to_string(i);
			std::ranges::replace_if(matName,[](const char c) { return c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|'; }, '_');
			
			std::vector<TextureToTrain> texturesToTrain;
			auto TryAddTexture{[&](MODEL_TEXTURE_TYPE _nekiType, aiTextureType _aiType, int _channels, float _weight, const std::string& _extract)
			{
				const std::pair<std::string, bool> texData{ GetMaterialTextureDataForSerialisation(assimpMaterial, static_cast<aiTextureTypeOverload>(_aiType), _nekiType) };
				const std::string& path{ texData.first };
				const bool isSRGB{ texData.second };
				if (!path.empty())
				{
					const std::string absPath{ std::filesystem::absolute(path).string() };
					texturesToTrain.push_back({ _nekiType, absPath, _channels, _weight, _extract, isSRGB });
				}
				return !path.empty();
			}};
			
			if (!TryAddTexture(MODEL_TEXTURE_TYPE::BASE_COLOUR, aiTextureType_BASE_COLOR, 3, 1.0f, "rgb"))
			{
				TryAddTexture(MODEL_TEXTURE_TYPE::DIFFUSE, aiTextureType_DIFFUSE, 3, 1.0f, "rgb");
			}
			if (!TryAddTexture(MODEL_TEXTURE_TYPE::NORMAL, aiTextureType_NORMAL_CAMERA, 3, 3.0f, "rgb"))
			{
				TryAddTexture(MODEL_TEXTURE_TYPE::NORMAL, aiTextureType_NORMALS, 3, 3.0f, "rgb");
			}
			if (!TryAddTexture(MODEL_TEXTURE_TYPE::EMISSIVE, aiTextureType_EMISSIVE, 3, 1.0f, "rgb"))
			{
				TryAddTexture(MODEL_TEXTURE_TYPE::EMISSION_COLOUR, aiTextureType_EMISSION_COLOR, 3, 1.0f, "rgb");
			}
			if (!TryAddTexture(MODEL_TEXTURE_TYPE::AMBIENT_OCCLUSION, aiTextureType_AMBIENT_OCCLUSION, 1, 1.0f, "r"))
			{
				if (!TryAddTexture(MODEL_TEXTURE_TYPE::LIGHTMAP, aiTextureType_LIGHTMAP, 1, 1.0f, "r"))
				{
					TryAddTexture(MODEL_TEXTURE_TYPE::AMBIENT, aiTextureType_AMBIENT, 1, 1.0f, "r");
				}
			}
			TryAddTexture(MODEL_TEXTURE_TYPE::DISPLACEMENT, aiTextureType_DISPLACEMENT, 1, 1.0f, "r");
			TryAddTexture(MODEL_TEXTURE_TYPE::ROUGHNESS, aiTextureType_DIFFUSE_ROUGHNESS, 1, 1.0f, "g");
			TryAddTexture(MODEL_TEXTURE_TYPE::METALNESS, aiTextureType_METALNESS, 1, 1.0f, "b");
			
			if (texturesToTrain.empty())
			{
				//Fallback: This material has no textures. Return it as a standard CPUMaterial.
				std::cout << "[NTC] Material '" << matName << "' has no textures suitable for NTC. Falling back to standard PBR material.\n";
				CPUMaterial standardMat;
				standardMat.name = matName;
				standardMat.pipeline = LIGHTING_MODEL::PHYSICALLY_BASED;
				PBRMaterial pbr{};
				std::memset(&pbr, 0, sizeof(PBRMaterial));
				standardMat.shaderMaterialData = pbr;
				materials[i] = standardMat;
				continue;
			}
			
			
			//Create the CLI command
			const std::string ptFilename{ matName + ".pt" };
			const std::string outPtPath{ _serialisedModelOutputDirectory + "/" + ptFilename };
			std::string scriptPath = std::string(NEKI_SOURCE_DIR) + "/util/ntc/train.py";
			std::string cmd{ "" };
			cmd += "\"" + std::string(NEKI_PYTHON_EXECUTABLE) + "\" -u \"" + scriptPath + "\"";
			cmd += " --out \"" + outPtPath + "\"";
			cmd += " --quality " + std::to_string(_neuralTrainingParameters.quality);
			cmd += " --hidden_neurons " + std::to_string(_neuralTrainingParameters.hiddenNeurons);
			cmd += " --epochs " + std::to_string(_neuralTrainingParameters.epochs);
			if (_flipTextures) { cmd += " --flip"; }
			
			std::string texArgs{ " --textures" };
			std::string chanArgs{ " --channels" };
			std::string weightArgs{ " --weights" };
			std::string extractArgs{ " --extract" };
			std::string convertToLinearArgs{ " --convert_to_linear" };
			
			PBRMaterialNTC pbrNTC{};
			std::memset(&pbrNTC, 0, sizeof(PBRMaterialNTC));
			std::size_t currentChannel{ 0 };
			for (const TextureToTrain& t : texturesToTrain)
			{
				texArgs += " \"" + t.path + "\"";
				chanArgs += " " + std::to_string(t.channels);
				extractArgs += " " + t.extract;
				convertToLinearArgs += " " + std::to_string(t.convertToLinear ? 1 : 0);
				for (std::size_t c{ 0 }; c < t.channels; ++c)
				{
					weightArgs += " " + std::to_string(t.weight);
				}

				switch (t.type)
				{
				case MODEL_TEXTURE_TYPE::BASE_COLOUR:
				case MODEL_TEXTURE_TYPE::DIFFUSE:
					pbrNTC.baseColourChannelR = currentChannel;
					pbrNTC.baseColourChannelG = currentChannel + 1;
					pbrNTC.baseColourChannelB = currentChannel + 2;
					pbrNTC.hasBaseColourChannelR = 1;
					pbrNTC.hasBaseColourChannelG = 1;
					pbrNTC.hasBaseColourChannelB = 1;
					break;
				case MODEL_TEXTURE_TYPE::NORMAL:
					pbrNTC.normalChannelX = currentChannel;
					pbrNTC.normalChannelY = currentChannel + 1;
					pbrNTC.normalChannelZ = currentChannel + 2;
					pbrNTC.hasNormalChannelX = 1;
					pbrNTC.hasNormalChannelY = 1;
					pbrNTC.hasNormalChannelZ = 1;
					break;
				case MODEL_TEXTURE_TYPE::EMISSIVE:
				case MODEL_TEXTURE_TYPE::EMISSION_COLOUR:
					pbrNTC.emissiveChannelR = currentChannel;
					pbrNTC.emissiveChannelG = currentChannel + 1;
					pbrNTC.emissiveChannelB = currentChannel + 2;
					pbrNTC.hasEmissiveChannelR = 1;
					pbrNTC.hasEmissiveChannelG = 1;
					pbrNTC.hasEmissiveChannelB = 1;
					break;
				case MODEL_TEXTURE_TYPE::AMBIENT_OCCLUSION:
				case MODEL_TEXTURE_TYPE::LIGHTMAP:
				case MODEL_TEXTURE_TYPE::AMBIENT:
					pbrNTC.aoChannel = currentChannel;
					pbrNTC.hasAoChannel = 1;
					break;
				case MODEL_TEXTURE_TYPE::DISPLACEMENT:
					pbrNTC.displacementChannel = currentChannel;
					pbrNTC.hasDisplacementChannel = 1;
					break;
				case MODEL_TEXTURE_TYPE::ROUGHNESS:
					pbrNTC.roughnessChannel = currentChannel;
					pbrNTC.hasRoughnessChannel = 1;
					break;
				case MODEL_TEXTURE_TYPE::METALNESS:
					pbrNTC.metalnessChannel = currentChannel;
					pbrNTC.hasMetalnessChannel = 1;
					break;
				default:
					break;
				}
				currentChannel += t.channels;
			}
			cmd += texArgs + chanArgs + weightArgs + extractArgs;
			cmd += " 2>&1";
			
			std::cout << "[NTC] Training neural material: " << matName << " (" << i+1 << "/" << scene->mNumMaterials << ")" << "...\n";
			std::cout << "[NTC] Generated command: " << cmd << '\n';
			FILE* const pipe{ POPEN(cmd.c_str(), "r") };
			if (!pipe)
			{
				throw std::runtime_error("Failed to open pipe for NTC training subprocess.");
			}
			char buffer[512];
			while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
			{
				std::cout << "[Python] " << buffer;
			}
			const int err{ PCLOSE(pipe) };
			if (err != 0)
			{
				throw std::runtime_error("NTC training failed with exit code: " + std::to_string(err));
			}
			
			CPUMaterialNTC ntcMat;
			ntcMat.name = matName;
			ntcMat.pipeline = LIGHTING_MODEL::PHYSICALLY_BASED;
			ntcMat.materialDataFilepath = outPtPath;
			ntcMat.numChannels = currentChannel;
			ntcMat.shaderMaterialData = pbrNTC;
			materials[i] = ntcMat;
		}
		
		//ProcessMesh hardcodes the .nkmaterial extension into the materialFilepath field even if it should be .nkmaterialntc, so it needs to be fixed
		//todo: find a better way of doing this - it's tricky though as it cannot be assumed that all meshes in the LoadNonNKModelDataNTC function will have an NTC material (as some materials may be deemed unsuitable for NTC)
		for (CPUMeshData& mesh : cpuMeshData)
		{
			const std::filesystem::path meshMatPath{ mesh.materialFilepath };
			const std::string stemName{ meshMatPath.stem().string() };

			for (const std::variant<CPUMaterial, CPUMaterialNTC>& matVariant : materials)
			{
				if (std::holds_alternative<CPUMaterialNTC>(matVariant))
				{
					const CPUMaterialNTC& ntcMat{ std::get<CPUMaterialNTC>(matVariant) };
					if (ntcMat.name == stemName)
					{
						mesh.materialFilepath = _serialisedModelOutputDirectory + "/" + stemName + ".nkmaterialntc";
						break;
					}
				}
			}
		}
		
		return std::make_pair<std::vector<CPUMeshData>, std::vector<std::variant<CPUMaterial, CPUMaterialNTC>>>(std::move(cpuMeshData), std::move(materials));
	}

	

	void ModelLoader::ProcessNode(const aiNode* _node, const aiScene* _scene, std::vector<CPUMeshData>* _outMeshData, const std::string& _outputMaterialDirectory)
	{
		//Process all the node's meshes (if any)
		for (std::size_t i{ 0 }; i < _node->mNumMeshes; ++i)
		{
			aiMesh* mesh{ _scene->mMeshes[_node->mMeshes[i]] };
			_outMeshData->push_back(ProcessMesh(mesh, _scene, _outputMaterialDirectory));
		}

		//Recursively process each child node
		for (std::size_t i{ 0 }; i < _node->mNumChildren; ++i)
		{
			ProcessNode(_node->mChildren[i], _scene, _outMeshData, _outputMaterialDirectory);
		}
	}



	CPUMeshData ModelLoader::ProcessMesh(aiMesh* _mesh, const aiScene* _scene, const std::string& _outputMaterialDirectory)
	{
		CPUMeshData cpuMesh;
		
		//Process vertices
		for (std::size_t i{ 0 }; i < _mesh->mNumVertices; ++i)
		{
			Vertex vertex{};

			//Position
			vertex.position = { _mesh->mVertices[i].x, _mesh->mVertices[i].y, _mesh->mVertices[i].z };

			//Normal
			if (_mesh->HasNormals())
			{
				vertex.normal = { _mesh->mNormals[i].x, _mesh->mNormals[i].y, _mesh->mNormals[i].z };
			}

			//Assimp allows up to 8 texture coordinates per vertex. Neki only supports the first set
			if (_mesh->mTextureCoords[0])
			{
				vertex.texCoord = { _mesh->mTextureCoords[0][i].x, _mesh->mTextureCoords[0][i].y };
			}

			//Tangent and Bitangent
			if (_mesh->HasTangentsAndBitangents())
			{
				glm::vec3 n(_mesh->mNormals[i].x, _mesh->mNormals[i].y, _mesh->mNormals[i].z);
				glm::vec3 t(_mesh->mTangents[i].x, _mesh->mTangents[i].y, _mesh->mTangents[i].z);
				glm::vec3 b(_mesh->mBitangents[i].x, _mesh->mBitangents[i].y, _mesh->mBitangents[i].z);

				float sign{ glm::dot(glm::cross(n, t), b) < 0.0f ? -1.0f : 1.0f };
				
				vertex.tangent   = { t.x, t.y, t.z, sign };
			}

			cpuMesh.vertices.push_back(vertex);
		}


		//Process indices
		for (std::size_t i{ 0 }; i < _mesh->mNumFaces; ++i)
		{
			//Append all indices on the face to our indices vector
			aiFace face{ _mesh->mFaces[i] };
			for (std::size_t j{ 0 }; j < face.mNumIndices; ++j)
			{
				cpuMesh.indices.push_back(face.mIndices[j]);
			}
		}
		
		std::string matName{ _scene->mMaterials[_mesh->mMaterialIndex]->GetName().C_Str() };
		if (matName.empty()) { matName = "Material"; }
		matName += "_" + std::to_string(_mesh->mMaterialIndex);
		std::ranges::replace_if(matName,[](const char c) { return c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|'; }, '_');

		cpuMesh.materialFilepath = _outputMaterialDirectory + std::string("/") + matName + std::string(".nkmaterial");

		return cpuMesh;
	}



	std::pair<std::string, bool> ModelLoader::GetMaterialTextureDataForSerialisation(aiMaterial* _material, aiTextureTypeOverload _assimpType, MODEL_TEXTURE_TYPE _nekiType)
	{
		const aiTextureType assimpType{ static_cast<aiTextureType>(_assimpType) };
		
		if (_material->GetTextureCount(assimpType) == 0)
		{
			return {};
		}

		//Load first texture of type (multiple textures of same type for same material is not currently supported by Neki)
		aiString assimpFilepath;
		_material->GetTexture(assimpType, 0, &assimpFilepath);
		std::string filepath{ assimpFilepath.C_Str() };
		
		//Replace all \ with /
		std::ranges::replace(filepath, '\\', '/');
		filepath = std::filesystem::path(filepath).replace_extension(".png").string();
		
		auto isColour = [&]()
		{
			switch (_nekiType) {
			case MODEL_TEXTURE_TYPE::DIFFUSE:
			case MODEL_TEXTURE_TYPE::SPECULAR:
			case MODEL_TEXTURE_TYPE::AMBIENT:
			case MODEL_TEXTURE_TYPE::EMISSIVE:
			case MODEL_TEXTURE_TYPE::EMISSION_COLOUR:
			case MODEL_TEXTURE_TYPE::BASE_COLOUR:
			case MODEL_TEXTURE_TYPE::REFLECTION:
				return true;
			default: return false;
			}
		};

		return { filepath, isColour() };
	}

}