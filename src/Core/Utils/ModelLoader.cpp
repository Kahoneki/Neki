#include "ModelLoader.h"

#include "FileUtils.h"
#include "ImageLoader.h"
#include "TextureCompressor.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cereal/archives/binary.hpp>


namespace NK
{

	std::unordered_map<std::string, CPUModel> ModelLoader::m_filepathToModelDataCache;



	const CPUModel* ModelLoader::LoadModel(const std::string& _filepath, bool _flipFaceWinding, bool _flipTextures)
	{
		const std::unordered_map<std::string, CPUModel>::iterator it{ m_filepathToModelDataCache.find(_filepath) };
		if (it != m_filepathToModelDataCache.end())
		{
			//Model has already been loaded, pull from cache
			return &(it->second);
		}
		
		if (FileUtils::GetFileExtension(_filepath) == ".nkmodel")
		{
			return LoadNKModel(_filepath);
		}
		return std::get<CPUModel*>(LoadNonNKModel(_filepath, _flipFaceWinding, _flipTextures, false));
	}



	void ModelLoader::UnloadModel(const std::string& _filepath)
	{
		if (!m_filepathToModelDataCache.contains(_filepath))
		{
			throw std::invalid_argument("ModelLoader::UnloadModel() - _filepath not in cache");
		}
		m_filepathToModelDataCache.erase(_filepath);
	}



	VertexInputDesc ModelLoader::GetModelVertexInputDescription()
	{
		std::vector<VertexAttributeDesc> vertexAttributes;
		
		//Position attribute
		VertexAttributeDesc posAttribute{};
		posAttribute.attribute = SHADER_ATTRIBUTE::POSITION;
		posAttribute.binding = 0;
		posAttribute.format = DATA_FORMAT::R32G32B32_SFLOAT;
		posAttribute.offset = offsetof(ModelVertex, position);
		vertexAttributes.push_back(posAttribute);

		//Normal attribute
		VertexAttributeDesc normAttribute{};
		normAttribute.attribute = SHADER_ATTRIBUTE::NORMAL;
		normAttribute.binding = 0;
		normAttribute.format = DATA_FORMAT::R32G32B32_SFLOAT;
		normAttribute.offset = offsetof(ModelVertex, normal);
		vertexAttributes.push_back(normAttribute);

		//Texcoord attribute
		VertexAttributeDesc uvAttribute{};
		uvAttribute.attribute = SHADER_ATTRIBUTE::TEXCOORD_0;
		uvAttribute.binding = 0;
		uvAttribute.format = DATA_FORMAT::R32G32_SFLOAT;
		uvAttribute.offset = offsetof(ModelVertex, texCoord);
		vertexAttributes.push_back(uvAttribute);

		//Tangent attribute
		VertexAttributeDesc tanAttribute{};
		tanAttribute.attribute = SHADER_ATTRIBUTE::TANGENT;
		tanAttribute.binding = 0;
		tanAttribute.format = DATA_FORMAT::R32G32B32A32_SFLOAT;
		tanAttribute.offset = offsetof(ModelVertex, tangent);
		vertexAttributes.push_back(tanAttribute);

		//Vertex buffer binding
		std::vector<VertexBufferBindingDesc> bufferBindings;
		VertexBufferBindingDesc bufferBinding{};
		bufferBinding.binding = 0;
		bufferBinding.inputRate = VERTEX_INPUT_RATE::VERTEX;
		bufferBinding.stride = sizeof(ModelVertex);
		bufferBindings.push_back(bufferBinding);

		//Vertex input description
		VertexInputDesc vertexInputDesc{};
		vertexInputDesc.attributeDescriptions = vertexAttributes;
		vertexInputDesc.bufferBindingDescriptions = bufferBindings;

		return vertexInputDesc;
	}



	void ModelLoader::SerialiseNKModel(const std::string& _inputFilepath, const std::string& _outputFilepath, bool _flipFaceWinding, bool _flipTextures)
	{
		//Create output filepath if it doesn't exist
		const std::filesystem::path outputPath{ _outputFilepath };
		const std::filesystem::path outputDir{ outputPath.parent_path() };
		if (!outputDir.empty())
		{
			std::filesystem::create_directories(outputDir);
		}

		//Load the serialised model into the archive
		std::ofstream os(_outputFilepath, std::ios::binary);
		if (!os)
		{
			throw std::runtime_error("ModelLoader::SerialiseNKModel() - failed to open _outputFilepath for writing. _outputFilepath (" + _outputFilepath + ")");
		}
		cereal::BinaryOutputArchive archive(os);
		CPUModel_Serialised model{ std::get<CPUModel_Serialised>(LoadNonNKModel(_inputFilepath, _flipFaceWinding, _flipTextures, true, outputDir)) };
		model.header.flipTextures = _flipTextures;
		archive(model);
	}



	CPUModel_SerialisedHeader ModelLoader::GetNKModelHeader(const std::string& _filepath)
	{
		std::ifstream fs(_filepath, std::ios::binary);
		if (!fs)
		{
			throw std::invalid_argument("ModelLoader::GetNKModelHeader() - failed to open model at _filepath (" + _filepath + ")");
		}
		cereal::BinaryInputArchive archive(fs);

		CPUModel_SerialisedHeader header{};
		archive(header);
		
		return header;
	}



	const CPUModel* ModelLoader::LoadNKModel(const std::string& _filepath)
	{
		std::ifstream fs(_filepath, std::ios::binary);
		if (!fs)
		{
			throw std::invalid_argument("ModelLoader::LoadModel() - failed to open model at _filepath (" + _filepath + ")");
		}
		cereal::BinaryInputArchive archive(fs);

		CPUModel_Serialised serialisedModel;
		archive(serialisedModel);

		CPUModel& model{ m_filepathToModelDataCache[_filepath] };
		model.meshes = std::move(serialisedModel.meshes);
		model.materials.resize(serialisedModel.materials.size());
		for (std::size_t matIndex{ 0 }; matIndex < serialisedModel.materials.size(); ++matIndex)
		{
			model.materials[matIndex].lightingModel = serialisedModel.materials[matIndex].lightingModel;
			model.materials[matIndex].shaderMaterialData = serialisedModel.materials[matIndex].shaderMaterialData;

			for (std::size_t texIndex{ 0 }; texIndex < serialisedModel.materials[matIndex].allTextures.size(); ++texIndex)
			{
				const std::pair<std::string, bool> texLoadData{ serialisedModel.materials[matIndex].allTextures[texIndex] };
				if (!texLoadData.first.empty())
				{
					model.materials[matIndex].allTextures[texIndex] = TextureCompressor::LoadImage(std::filesystem::path(_filepath).parent_path().string() + "/" + texLoadData.first, serialisedModel.header.flipTextures, texLoadData.second);
					model.materials[matIndex].allTextures[texIndex]->desc.usage |= TEXTURE_USAGE_FLAGS::READ_ONLY;
				}
			}
		}

		return &(m_filepathToModelDataCache[_filepath]);
	}



	std::variant<CPUModel*, CPUModel_Serialised> ModelLoader::LoadNonNKModel(const std::string& _filepath, bool _flipFaceWinding, bool _flipTextures, bool _serialisedModelOutput, const std::filesystem::path& _serialisedModelTextureOutputDirectory)
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

		CPUModel_Serialised cpuModelSerialised{};
		CPUModel cpuModel{};
		std::string modelDirectory{ _filepath.substr(0, _filepath.find_last_of('/')) };
		if (_serialisedModelOutput) { ProcessNode(scene->mRootNode, scene, &cpuModelSerialised, modelDirectory); }
		else { ProcessNode(scene->mRootNode, scene, &cpuModel, modelDirectory); }
		
		//Calculate model extents
		glm::vec3 minAABB(std::numeric_limits<float>::max());
		glm::vec3 maxAABB(std::numeric_limits<float>::lowest());
		auto calculateExtentsAndCenterModel{ [&](auto* _model)
		{
			//Calculate extents
			for (const CPUMesh& mesh : _model->meshes)
			{
				for (const ModelVertex& vertex : mesh.vertices)
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
			for (CPUMesh& mesh : _model->meshes)
			{
				for (ModelVertex& vertex : mesh.vertices)
				{
					vertex.position -= extentsCentre;
				}
			}
		} };

		if (_serialisedModelOutput)
		{
			calculateExtentsAndCenterModel(&cpuModelSerialised);
			cpuModelSerialised.header.halfExtents = (maxAABB - minAABB) * 0.5f;
		}
		else
		{
			calculateExtentsAndCenterModel(&cpuModel);
			cpuModel.halfExtents = (maxAABB - minAABB) * 0.5f;
		}
		

		//Load scene materials
		if (_serialisedModelOutput) { cpuModelSerialised.materials.resize(scene->mNumMaterials); }
		else { cpuModel.materials.resize(scene->mNumMaterials); }
		for (std::size_t i{ 0 }; i < scene->mNumMaterials; ++i)
		{
			aiMaterial* assimpMaterial{ scene->mMaterials[i] };
			CPUMaterial_Serialised nekiMaterialSerialised{};
			CPUMaterial nekiMaterial{};

			auto load{ [&](const MODEL_TEXTURE_TYPE _dst, const aiTextureType _src)
			{
				if (_serialisedModelOutput)
				{
					nekiMaterialSerialised.allTextures[std::to_underlying(_dst)] = GetMaterialTextureDataForSerialisation(assimpMaterial, static_cast<aiTextureTypeOverload>(_src), _dst, modelDirectory);

					if (!nekiMaterialSerialised.allTextures.at(std::to_underlying(_dst)).first.empty())
					{
						//Compress to .ktx2
						std::string& filepath{ nekiMaterialSerialised.allTextures.at(std::to_underlying(_dst)).first };
						const std::string newFilepath{ (_serialisedModelTextureOutputDirectory / std::filesystem::path(filepath).filename()).replace_extension(".ktx2").string() };
						TextureCompressor::KTXCompress(filepath, nekiMaterialSerialised.allTextures.at(std::to_underlying(_dst)).second, _flipTextures, newFilepath);
						filepath = std::filesystem::path(newFilepath).filename().string(); //filepath is reference so this is modifying the lookup entry to point to the new ktx2 texture - just store the path relative to the model (so just the filename)
					}
				}
				else
				{
					nekiMaterial.allTextures[std::to_underlying(_dst)] = LoadMaterialTexture(assimpMaterial, static_cast<aiTextureTypeOverload>(_src), _dst, modelDirectory, _flipTextures);
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
			if ((nekiMaterial.allTextures[std::to_underlying(MODEL_TEXTURE_TYPE::NORMAL)] == nullptr) && (nekiMaterialSerialised.allTextures[std::to_underlying(MODEL_TEXTURE_TYPE::NORMAL)].first.empty()))
			{
				load(MODEL_TEXTURE_TYPE::NORMAL, aiTextureType_NORMALS);
			}


			//Determine lighting model
			auto hasTex{ [&](const MODEL_TEXTURE_TYPE _tex) { return (nekiMaterial.allTextures[std::to_underlying(_tex)] != nullptr) || (!nekiMaterialSerialised.allTextures[std::to_underlying(_tex)].first.empty()); } };
			const bool isPBR{	hasTex(MODEL_TEXTURE_TYPE::METALNESS)			||
								hasTex(MODEL_TEXTURE_TYPE::ROUGHNESS)			||
								hasTex(MODEL_TEXTURE_TYPE::BASE_COLOUR)			||
								hasTex(MODEL_TEXTURE_TYPE::AMBIENT_OCCLUSION)	||
								hasTex(MODEL_TEXTURE_TYPE::EMISSION_COLOUR) };

			if (_serialisedModelOutput) { nekiMaterialSerialised.lightingModel = isPBR ? LIGHTING_MODEL::PHYSICALLY_BASED : LIGHTING_MODEL::BLINN_PHONG; }
			else { nekiMaterial.lightingModel = isPBR ? LIGHTING_MODEL::PHYSICALLY_BASED : LIGHTING_MODEL::BLINN_PHONG; }


			//Populate shader material data
			switch (_serialisedModelOutput ? nekiMaterialSerialised.lightingModel : nekiMaterial.lightingModel)
			{
			case LIGHTING_MODEL::BLINN_PHONG:
			{
				if (_serialisedModelOutput) { nekiMaterialSerialised.shaderMaterialData = BlinnPhongMaterial{}; }
				else { nekiMaterial.shaderMaterialData = BlinnPhongMaterial{}; }
				BlinnPhongMaterial& material{ std::get<BlinnPhongMaterial>(_serialisedModelOutput ? nekiMaterialSerialised.shaderMaterialData : nekiMaterial.shaderMaterialData) };
				
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
				if (_serialisedModelOutput) { nekiMaterialSerialised.shaderMaterialData = PBRMaterial{}; }
				else { nekiMaterial.shaderMaterialData = PBRMaterial{}; }
				PBRMaterial& material{ std::get<PBRMaterial>(_serialisedModelOutput ? nekiMaterialSerialised.shaderMaterialData : nekiMaterial.shaderMaterialData) };

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

			
			if (_serialisedModelOutput) { cpuModelSerialised.materials[i] = nekiMaterialSerialised; }
			else { cpuModel.materials[i] = nekiMaterial; }
		}


		if (_serialisedModelOutput)
		{
			return std::move(cpuModelSerialised);
		}
		else
		{
			//Add to cache
			m_filepathToModelDataCache[_filepath] = cpuModel;
			return &(m_filepathToModelDataCache[_filepath]);
		}
	}



	void ModelLoader::ProcessNode(aiNode* _node, const aiScene* _scene, std::variant<CPUModel*, CPUModel_Serialised*> _outModel, const std::string& _modelDirectory)
	{
		//Process all the node's meshes (if any)
		for (std::size_t i{ 0 }; i < _node->mNumMeshes; ++i)
		{
			aiMesh* mesh{ _scene->mMeshes[_node->mMeshes[i]] };

			std::visit([&](auto* _model)
			{
				_model->meshes.push_back(ProcessMesh(mesh, _scene, _modelDirectory));
			}, _outModel);
		}

		//Recursively process each child node
		for (std::size_t i{ 0 }; i < _node->mNumChildren; ++i)
		{
			ProcessNode(_node->mChildren[i], _scene, _outModel, _modelDirectory);
		}
	}



	CPUMesh ModelLoader::ProcessMesh(aiMesh* _mesh, const aiScene* _scene, const std::string& _directory)
	{
		CPUMesh nekiMesh;

		
		//Process vertices
		for (std::size_t i{ 0 }; i < _mesh->mNumVertices; ++i)
		{
			ModelVertex vertex{};

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

			nekiMesh.vertices.push_back(vertex);
		}


		//Process indices
		for (std::size_t i{ 0 }; i < _mesh->mNumFaces; ++i)
		{
			//Append all indices on the face to our indices vector
			aiFace face{ _mesh->mFaces[i] };
			for (std::size_t j{ 0 }; j < face.mNumIndices; ++j)
			{
				nekiMesh.indices.push_back(face.mIndices[j]);
			}
		}


		nekiMesh.materialIndex = _mesh->mMaterialIndex;


		return nekiMesh;
	}



	ImageData* ModelLoader::LoadMaterialTexture(aiMaterial* _material, aiTextureTypeOverload _assimpType, MODEL_TEXTURE_TYPE _nekiType, const std::string& _directory, bool _flipTexture)
	{
		const aiTextureType assimpType{ static_cast<aiTextureType>(_assimpType) };
		
		if (_material->GetTextureCount(assimpType) == 0)
		{
			return {};
		}

		//Load first texture of type (multiple textures of same type for same material is not currently supported by Neki)
		aiString filename;
		_material->GetTexture(assimpType, 0, &filename);
		const std::string filepath{ _directory + "/" + filename.C_Str() };
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

		ImageData* const imageData{ ImageLoader::LoadImage(filepath, _flipTexture, isColour()) };
		imageData->desc.usage |= TEXTURE_USAGE_FLAGS::READ_ONLY;
		return imageData;
	}



	std::pair<std::string, bool> ModelLoader::GetMaterialTextureDataForSerialisation(aiMaterial* _material, aiTextureTypeOverload _assimpType, MODEL_TEXTURE_TYPE _nekiType, const std::string& _directory)
	{
		//todo: this should be refactored, too much overlap with LoadMaterialTexture()
		
		const aiTextureType assimpType{ static_cast<aiTextureType>(_assimpType) };
		
		if (_material->GetTextureCount(assimpType) == 0)
		{
			return {};
		}

		//Load first texture of type (multiple textures of same type for same material is not currently supported by Neki)
		aiString filename;
		_material->GetTexture(assimpType, 0, &filename);
		const std::string filepath{ _directory + "/" + filename.C_Str() };
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