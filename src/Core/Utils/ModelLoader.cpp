#include "ModelLoader.h"
#include <stdexcept>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "ImageLoader.h"

namespace NK
{

	std::unordered_map<std::string, CPUModel> ModelLoader::m_filepathToModelDataCache;



	const CPUModel* const ModelLoader::LoadModel(const std::string& _filepath, bool _flipFaceWinding, bool _flipTextures)
	{
		const std::unordered_map<std::string, CPUModel>::iterator it{ m_filepathToModelDataCache.find(_filepath) };
		if (it != m_filepathToModelDataCache.end())
		{
			//Model has already been loaded, pull from cache
			return &(it->second);
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

		CPUModel model{};
		std::string modelDirectory{ _filepath.substr(0, _filepath.find_last_of('/')) };
		ProcessNode(scene->mRootNode, scene, model, modelDirectory);

		//Load scene materials
		model.materials.resize(scene->mNumMaterials);
		for (std::size_t i{ 0 }; i < scene->mNumMaterials; ++i)
		{
			aiMaterial* assimpMaterial{ scene->mMaterials[i] };
			CPUMaterial nekiMaterial{};

			auto load{ [&](const MODEL_TEXTURE_TYPE _dst, const aiTextureType _src)
			{
				nekiMaterial.allTextures[std::to_underlying(_dst)] = LoadMaterialTexture(assimpMaterial, static_cast<aiTextureTypeOverload>(_src), _dst, modelDirectory, _flipTextures);
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
			load(MODEL_TEXTURE_TYPE::BASE_COLOUR       , aiTextureType_BASE_COLOR);
			load(MODEL_TEXTURE_TYPE::METALNESS        , aiTextureType_METALNESS);
			load(MODEL_TEXTURE_TYPE::ROUGHNESS        , aiTextureType_DIFFUSE_ROUGHNESS);
			load(MODEL_TEXTURE_TYPE::EMISSION_COLOUR   , aiTextureType_EMISSION_COLOR);
			load(MODEL_TEXTURE_TYPE::AMBIENT_OCCLUSION, aiTextureType_AMBIENT_OCCLUSION);

			//If NORMAL_CAMERA wasn't available, fall back to NORMAL
			if (nekiMaterial.allTextures[std::to_underlying(MODEL_TEXTURE_TYPE::NORMAL)] == nullptr)
			{
				load(MODEL_TEXTURE_TYPE::NORMAL, aiTextureType_NORMALS);
			}


			//Determine lighting model
			auto hasTex{ [&](const MODEL_TEXTURE_TYPE _tex) { return nekiMaterial.allTextures[std::to_underlying(_tex)] != nullptr; } };
			const bool isPBR{	hasTex(MODEL_TEXTURE_TYPE::METALNESS)			||
								hasTex(MODEL_TEXTURE_TYPE::ROUGHNESS)			||
								hasTex(MODEL_TEXTURE_TYPE::BASE_COLOUR)			||
								hasTex(MODEL_TEXTURE_TYPE::AMBIENT_OCCLUSION)	||
								hasTex(MODEL_TEXTURE_TYPE::EMISSION_COLOUR) };

			nekiMaterial.lightingModel = isPBR ? LIGHTING_MODEL::PHYSICALLY_BASED : LIGHTING_MODEL::BLINN_PHONG;


			//Populate shader material data
			switch (nekiMaterial.lightingModel)
			{
			case LIGHTING_MODEL::BLINN_PHONG:
			{
				nekiMaterial.shaderMaterialData = BlinnPhongMaterial{};
				BlinnPhongMaterial& material{ std::get<BlinnPhongMaterial>(nekiMaterial.shaderMaterialData) };
				
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
				nekiMaterial.shaderMaterialData = PBRMaterial{};
				PBRMaterial& material{ std::get<PBRMaterial>(nekiMaterial.shaderMaterialData) };

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
			

			model.materials[i] = nekiMaterial;
		}
		
		//Add to cache
		m_filepathToModelDataCache[_filepath] = model;
		
		return &(m_filepathToModelDataCache[_filepath]);
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
		tanAttribute.format = DATA_FORMAT::R32G32B32_SFLOAT;
		tanAttribute.offset = offsetof(ModelVertex, tangent);
		vertexAttributes.push_back(tanAttribute);

		//Bitangent attribute
		VertexAttributeDesc bitanAttribute{};
		bitanAttribute.attribute = SHADER_ATTRIBUTE::BITANGENT;
		bitanAttribute.binding = 0;
		bitanAttribute.format = DATA_FORMAT::R32G32B32_SFLOAT;
		bitanAttribute.offset = offsetof(ModelVertex, bitangent);
		vertexAttributes.push_back(bitanAttribute);

		//Vertex buffer binding
		std::vector<VertexBufferBindingDesc> bufferBindings;
		VertexBufferBindingDesc bufferBinding{};
		bufferBinding.binding = 0;
		bufferBinding.inputRate = NK::VERTEX_INPUT_RATE::VERTEX;
		bufferBinding.stride = sizeof(ModelVertex);
		bufferBindings.push_back(bufferBinding);

		//Vertex input description
		VertexInputDesc vertexInputDesc{};
		vertexInputDesc.attributeDescriptions = vertexAttributes;
		vertexInputDesc.bufferBindingDescriptions = bufferBindings;

		return vertexInputDesc;
	}
	
}