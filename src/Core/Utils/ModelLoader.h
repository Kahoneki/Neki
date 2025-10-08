#pragma once
#include "ImageLoader.h"
#include <array>

//Forward declaration for Assimp types to avoid including Assimp headers in public Neki library
struct aiNode;
struct aiMesh;
struct aiScene;
struct aiMaterial;

namespace NK
{
	
	enum class MODEL_TEXTURE_TYPE : std::size_t
	{
		//Key:
		//Y = yes - accepted as a native part of the lighting model
		//S = sometimes
		//N = no - ignored in the lighting model
		
		//Name						┌─ Blinn-Phong ──────┐  ┌─ PBR Spec-Gloss ───────────┐  ┌─ PBR Metal-Rough ───────┐
		DIFFUSE				= 0,	//Y						//S (fallback)					//S (fallback)
		SPECULAR			= 1,	//Y						//Y								//N
		AMBIENT				= 2,	//S (AO tint)			//S (AO)						//S (AO)
		EMISSIVE			= 3,	//Y						//Y								//Y
		HEIGHT				= 4,	//S (non-lighting)		//S (non-lighting)				//S (non-lighting)
		NORMAL				= 5,	//Y						//Y								//Y
		SHININESS			= 6,	//Y						//Y								//S (convert -> roughness)
		OPACITY				= 7,	//Y						//Y								//Y
		DISPLACEMENT		= 8,	//S						//S								//S
		LIGHTMAP			= 9,	//S						//Y (AO)						//Y (AO)
		REFLECTION			= 10,	//S (static env-map)	//S (optional env-map)			//S (optional env-map)
		BASE_COLOR			= 11,	//N						//S (preferred over DIFFUSE)	//Y
		METALNESS			= 12,	//N						//N								//Y
		ROUGHNESS			= 13,	//N						//N								//Y
		EMISSION_COLOR		= 14,	//Y						//Y								//Y
		AMBIENT_OCCLUSION	= 15,	//S						//Y								//Y

		NUM_MODEL_TEXTURE_TYPES = 16,
	};
	
	
	struct ModelVertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texCoord;
		glm::vec3 tangent;
		glm::vec3 bitangent;
	};
	
	struct CPUMesh
	{
		std::vector<ModelVertex> vertices;
		std::vector<std::uint32_t> indices;
		std::size_t materialIndex;
	};

	enum class LIGHTING_MODEL
	{
		BLINN_PHONG,
		PBR_SPEC_GLOSS,
		PBR_METAL_ROUGH,
	};
	
	struct TexturesOfType
	{
		MODEL_TEXTURE_TYPE type;
		std::vector<ImageData*> textures; //Textures of this type
	};
	
	struct CPUMaterial
	{
		std::array<TexturesOfType, std::to_underlying(MODEL_TEXTURE_TYPE::NUM_MODEL_TEXTURE_TYPES)> allTextures; //Every texture for every texture type for this material
	};
	
	struct CPUModel
	{
		std::vector<CPUMesh> meshes;
		std::vector<CPUMaterial> materials;
	};


	//To avoid having to include assimp headers in public Neki library
	enum aiTextureTypeOverload{};


	class ModelLoader final
	{
	public:
		[[nodiscard]] static CPUModel* LoadModel(const std::string& _filepath, bool _flipFaceWinding, bool _flipTextures);


	private:
		//Recursively process nodes in the Assimp scene graph
		static void ProcessNode(aiNode* _node, const aiScene* _scene, CPUModel& _outModel, const std::string& _modelDirectory);

		//Translate an Assimp mesh to an NK::Mesh
		static CPUMesh ProcessMesh(aiMesh* _mesh, const aiScene* _scene, const std::string& _directory);

		static TexturesOfType LoadMaterialTextures(aiMaterial* _material, aiTextureTypeOverload _assimpType, MODEL_TEXTURE_TYPE _nekiType, const std::string& _directory, bool _flipTextures);
		
		
		//To avoid unnecessary duplicate loads
		static std::unordered_map<std::string, UniquePtr<CPUModel>> m_filepathToModelDataCache;
	};
	
}