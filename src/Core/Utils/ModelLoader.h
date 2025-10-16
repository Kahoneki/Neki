#pragma once

#include "ImageLoader.h"

#include <Core/Memory/Allocation.h>
#include <RHI/IPipeline.h>
#include <Types/Materials.h>

#include <array>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
#include <glm/glm.hpp>


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
		BASE_COLOUR			= 11,	//N						//S (preferred over DIFFUSE)	//Y
		METALNESS			= 12,	//N						//N								//Y
		ROUGHNESS			= 13,	//N						//N								//Y
		EMISSION_COLOUR		= 14,	//Y						//Y								//Y
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
		PHYSICALLY_BASED,
	};
	
	struct CPUMaterial
	{
		LIGHTING_MODEL lightingModel;
		std::array<ImageData*, std::to_underlying(MODEL_TEXTURE_TYPE::NUM_MODEL_TEXTURE_TYPES)> allTextures; //Every texture for every texture type for this material (limited to 1 texture per texture type per material)

		//There's a bit of translation going on here to communicate to the GPUUploader
		//The bool flags in the material type will be populated by ModelLoader with the actual correct values that will end up going to the GPU
		//The index members in the material type will be populated with MODEL_TEXTURE_TYPE values, representing the texture type the GPUUploader class should populate the texture slot with
		//
		//This is a convenient workaround for the fact that there isn't a 1-to-1 mapping between the MODEL_TEXTURE_TYPE enum and the texture types represented in the material types
		//For example, BlinnPhongMaterial has `hasEmissive` and `emissiveIdx` members, but if the material provides both an EMISSIVE and EMISSION_COLOUR type, which one should these fields refer to?
		//In the specific case above, it will favour EMISSION_COLOUR - this decision will be specific to each texture type that has this problem
		//This choice is decided by the ModelLoader which then passes on this information the GPUUploader by populating the emissiveIdx field with MODEL_TEXTURE_TYPE::EMISSIVE or EMISSION_COLOUR so the GPU knows which texture to pull.
		//The GPUUploader, will update these values with the indices it allocates when creating the GPUMaterial
		std::variant<BlinnPhongMaterial, PBRMaterial> shaderMaterialData;
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
		[[nodiscard]] static const CPUModel* const LoadModel(const std::string& _filepath, bool _flipFaceWinding, bool _flipTextures);

		[[nodiscard]] static VertexInputDesc GetModelVertexInputDescription();


	private:
		//Recursively process nodes in the Assimp scene graph
		static void ProcessNode(aiNode* _node, const aiScene* _scene, CPUModel& _outModel, const std::string& _modelDirectory);

		//Translate an Assimp mesh to an NK::Mesh
		static CPUMesh ProcessMesh(aiMesh* _mesh, const aiScene* _scene, const std::string& _directory);

		static ImageData* LoadMaterialTexture(aiMaterial* _material, aiTextureTypeOverload _assimpType, MODEL_TEXTURE_TYPE _nekiType, const std::string& _directory, bool _flipTexture);
		
		
		//To avoid unnecessary duplicate loads
		static std::unordered_map<std::string, CPUModel> m_filepathToModelDataCache;
	};
	
}