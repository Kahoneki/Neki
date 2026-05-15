#pragma once

#include "ImageLoader.h"

#include "Serialisation/Serialisation.h"

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

	//To avoid having to include assimp headers in public Neki library
	enum aiTextureTypeOverload{};


	class ModelLoader final
	{
	public:
		[[nodiscard]] static const CPUMeshData* LoadMesh(const std::string& _filepath, std::uint64_t _offset);

		//Removes the specified mesh from the cache
		static void UnloadMesh(const std::string& _filepath, std::uint64_t _offset);
		
		[[nodiscard]] static std::variant<CPUMaterial, CPUMaterialNTC> GetMaterialHeader(const std::string& _filepath);
		
		//Serialise a model of any type (.gltf, .fbx, .obj, etc.) from _inputFilepath into a .nkmodel file at _outputFilepath
		static void SerialiseNKModel(const std::string& _inputFilepath, const std::string& _outputFilepath, bool _flipFaceWinding, bool _flipTextures);
		
		//Serialise a model of any type (.gltf, .fbx, .obj, etc.) from _inputFilepath into a .nkmodel file at _outputFilepath with neural materials
		static void SerialiseNKModelNTC(const std::string& _inputFilepath, const std::string& _outputFilepath, bool _flipFaceWinding, bool _flipTextures, const NeuralTrainingParameters& _neuralTrainingParameters);

		//Get the header of a .nkmodel
		static CPUModel GetNKModelHeader(const std::string& _filepath);
		
		[[nodiscard]] static VertexInputDesc GetModelVertexInputDescription();

		static void ClearCache();
		

	private:
		[[nodiscard]] static std::pair<std::vector<CPUMeshData>, std::vector<CPUMaterial>> LoadNonNKModelData(const std::string& _filepath, bool _flipFaceWinding, bool _flipTextures, const std::string& _serialisedModelOutputDirectory);
		[[nodiscard]] static std::pair<std::vector<CPUMeshData>, std::vector<std::variant<CPUMaterial, CPUMaterialNTC>>> LoadNonNKModelDataNTC(const std::string& _filepath, bool _flipFaceWinding, bool _flipTextures, const std::string& _serialisedModelOutputDirectory, const NeuralTrainingParameters& _neuralTrainingParameters);
		
		//Recursively process nodes in the Assimp scene graph
		static void ProcessNode(const aiNode* _node, const aiScene* _scene, std::vector<CPUMeshData>* _outMeshData, const std::string& _outputMaterialDirectory);

		//Translate an Assimp mesh to an NK::CPUMeshData
		static CPUMeshData ProcessMesh(aiMesh* _mesh, const aiScene* _scene, const std::string& _outputMaterialDirectory);

		static std::pair<std::string, bool> GetMaterialTextureDataForSerialisation(aiMaterial* _material, aiTextureTypeOverload _assimpType, MODEL_TEXTURE_TYPE _nekiType, const std::string& _materialDirectory); //std::pair of filepath and srgb-flag
		
		
		//To avoid unnecessary duplicate loads
		static std::map<std::pair<std::string, std::uint64_t>, UniquePtr<CPUMeshData>> m_filepathToMeshDataCache; //Key = pair of .nkmeshdata filepath and offset
	};
	
}