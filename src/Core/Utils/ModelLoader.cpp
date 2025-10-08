#include "ModelLoader.h"
#include <stdexcept>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace NK
{

	std::unordered_map<std::string, UniquePtr<CPUModel>> ModelLoader::m_filepathToModelDataCache;



	CPUModel* ModelLoader::LoadModel(const std::string& _filepath, bool _flipFaceWinding, bool _flipTextures)
	{
		const std::unordered_map<std::string, UniquePtr<CPUModel>>::iterator it{ m_filepathToModelDataCache.find(_filepath) };
		if (it != m_filepathToModelDataCache.end())
		{
			//Model has already been loaded, pull from cache
			return it->second.get();
		}
		
		Assimp::Importer importer{};
		const aiScene* scene{ importer.ReadFile(_filepath,
		                                        aiProcess_Triangulate |			//Ensure model is composed of triangles
		                                        aiProcess_GenSmoothNormals |	//Generate smooth normals if they don't exist
		                                        /*aiProcess_FlipUVs |*/			//Flip UVs to match Vulkan's top left texcoord system
		                                        aiProcess_CalcTangentSpace |	//Calculate tangents and bitangents (required for TBN in normal mapping)
		                                        (_flipFaceWinding ? aiProcess_FlipWindingOrder : 0)
		                                       ) };

		//Ensure scene was loaded correctly
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			throw std::runtime_error("ModelLoader::LoadModel() - Failed to load model (" + _filepath + ") - " + std::string(importer.GetErrorString()));
		}

		UniquePtr<CPUModel> model{ NK_NEW(CPUModel) };
		std::string modelDirectory{ _filepath.substr(0, _filepath.find_last_of('/')) };
		ProcessNode(scene->mRootNode, scene, *model, modelDirectory);

		//Load scene materials
		model->materials.resize(scene->mNumMaterials);
		for (std::size_t i{ 0 }; i < scene->mNumMaterials; ++i)
		{
			aiMaterial* assimpMaterial{ scene->mMaterials[i] };
			CPUMaterial nekiMaterial{};

			auto load{ [&](const MODEL_TEXTURE_TYPE dst, const aiTextureType src)
			{
				nekiMaterial.allTextures[std::to_underlying(dst)] = LoadMaterialTextures(assimpMaterial, static_cast<aiTextureTypeOverload>(src), dst, modelDirectory, _flipTextures);
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
			load(MODEL_TEXTURE_TYPE::BASE_COLOR       , aiTextureType_BASE_COLOR);
			load(MODEL_TEXTURE_TYPE::METALNESS        , aiTextureType_METALNESS);
			load(MODEL_TEXTURE_TYPE::ROUGHNESS        , aiTextureType_DIFFUSE_ROUGHNESS);
			load(MODEL_TEXTURE_TYPE::EMISSION_COLOR   , aiTextureType_EMISSION_COLOR);
			load(MODEL_TEXTURE_TYPE::AMBIENT_OCCLUSION, aiTextureType_AMBIENT_OCCLUSION);

			//If NORMAL_CAMERA wasn't available, fall back to NORMAL
			if (nekiMaterial.allTextures[std::to_underlying(MODEL_TEXTURE_TYPE::NORMAL)].textures.empty())
			{
				load(MODEL_TEXTURE_TYPE::NORMAL, aiTextureType_NORMALS);
			}

			//If BASE_COLOUR wasn't available, fall back to DIFFUSE
			if (nekiMaterial.allTextures[std::to_underlying(MODEL_TEXTURE_TYPE::BASE_COLOR)].textures.empty())
			{
				load(MODEL_TEXTURE_TYPE::BASE_COLOR, aiTextureType_DIFFUSE);
			}

			//If ROUGHNESS wasn't available, fall back to SHININESS
			if (nekiMaterial.allTextures[std::to_underlying(MODEL_TEXTURE_TYPE::SHININESS)].textures.empty())
			{
				load(MODEL_TEXTURE_TYPE::ROUGHNESS, aiTextureType_SHININESS);
			}

			model->materials.push_back(nekiMaterial);
		}
		
		//Add to cache
		m_filepathToModelDataCache[_filepath] = std::move(model);
		
		return m_filepathToModelDataCache[_filepath].get();
	}



	void ModelLoader::ProcessNode(aiNode* _node, const aiScene* _scene, CPUModel& _outModel, const std::string& _modelDirectory)
	{
		//Process all the node's meshes (if any)
		for (std::size_t i{ 0 }; i < _node->mNumMeshes; ++i)
		{
			aiMesh* mesh{ _scene->mMeshes[_node->mMeshes[i]] };
			_outModel.meshes.push_back(ProcessMesh(mesh, _scene, _modelDirectory));
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
				vertex.tangent = { _mesh->mTangents[i].x, _mesh->mTangents[i].y, _mesh->mTangents[i].z };
				vertex.bitangent = { _mesh->mBitangents[i].x, _mesh->mBitangents[i].y, _mesh->mBitangents[i].z };
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



	TexturesOfType ModelLoader::LoadMaterialTextures(aiMaterial* _material, aiTextureTypeOverload _assimpType, MODEL_TEXTURE_TYPE _nekiType, const std::string& _directory, bool _flipTextures)
	{
		const aiTextureType assimpType{ static_cast<aiTextureType>(_assimpType) };
		
		//Loop through all textures of type _assimpType for _material and load image data
		TexturesOfType textureInfo{};
		textureInfo.type = _nekiType;
		textureInfo.textures.resize(_material->GetTextureCount(assimpType));
		for (std::size_t i{ 0 }; i < textureInfo.textures.size(); ++i)
		{
			//Get file name of texture
			aiString filename;
			_material->GetTexture(assimpType, i, &filename);

			//Load image
			const std::string filepath{ _directory + "/" + filename.C_Str() };
			auto isColour = [&]()
			{
				switch (_nekiType) {
				case MODEL_TEXTURE_TYPE::DIFFUSE:
				case MODEL_TEXTURE_TYPE::SPECULAR:
				case MODEL_TEXTURE_TYPE::AMBIENT:
				case MODEL_TEXTURE_TYPE::EMISSIVE:
				case MODEL_TEXTURE_TYPE::EMISSION_COLOR:
				case MODEL_TEXTURE_TYPE::BASE_COLOR:
				case MODEL_TEXTURE_TYPE::REFLECTION:
					return true;
				default: return false;
				}
			};
			textureInfo.textures[i] = ImageLoader::LoadImage(filepath, _flipTextures, isColour());
		}
		return textureInfo;
	}

}