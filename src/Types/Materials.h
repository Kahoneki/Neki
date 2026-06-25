#ifndef NEKI_MATERIALS_H
#define NEKI_MATERIALS_H

#if defined(__cplusplus)
	#include <Core/Utils/Serialisation/Serialisation.h>
#endif


namespace NK
{
	//`hasX` fields are of type int for compatibility between C++ (bools aligned to 1-byte) and HLSL (bools aligned to 4-bytes)

	struct BlinnPhongMaterial
	{
		//Texture indices
		int diffuseIdx;
		int specularIdx;
		int ambientIdx;
		int emissiveIdx;
		int normalIdx;
		int shininessIdx;
		int opacityIdx;
		int heightIdx;
		int displacementIdx;
		int lightmapIdx;
		int reflectionIdx;

		//Presence flags (0 / 1)
		int hasDiffuse;
		int hasSpecular;
		int hasAmbient;
		int hasEmissive;
		int hasNormal;
		int hasShininess;
		int hasOpacity;
		int hasHeight;
		int hasDisplacement;
		int hasLightmap;
		int hasReflection;
	};
	#if defined(__cplusplus)
		SERIALISE(BlinnPhongMaterial, v.diffuseIdx, v.specularIdx, v.ambientIdx, v.emissiveIdx, v.normalIdx, v.shininessIdx, v.opacityIdx, v.heightIdx, v.displacementIdx, v.lightmapIdx, v.reflectionIdx, v.hasDiffuse, v.hasSpecular, v.hasAmbient, v.hasEmissive, v.hasNormal, v.hasShininess, v.hasOpacity, v.hasHeight, v.hasDisplacement, v.hasLightmap, v.hasReflection)
	#endif

	struct PBRMetallicRoughnessMaterial
	{
		//Texture indices
		int baseColourIdx;
		int metalnessIdx;
		int roughnessIdx;
		int specularIdx;
		int shininessIdx;
		int normalIdx;
		int aoIdx;
		int emissiveIdx;
		int opacityIdx;
		int heightIdx;
		int displacementIdx;
		int reflectionIdx;

		//Presence flags (0 / 1)
		int hasBaseColour;
		int hasMetalness;
		int hasRoughness;
		int hasSpecular;
		int hasShininess;
		int hasNormal;
		int hasAO;
		int hasEmissive;
		int hasOpacity;
		int hasHeight;
		int hasDisplacement;
		int hasReflection;
	};
	#if defined(__cplusplus)
		SERIALISE(PBRMetallicRoughnessMaterial, v.baseColourIdx, v.metalnessIdx, v.roughnessIdx, v.specularIdx, v.shininessIdx, v.normalIdx, v.aoIdx, v.emissiveIdx, v.opacityIdx, v.heightIdx, v.displacementIdx, v.reflectionIdx, v.hasBaseColour, v.hasMetalness, v.hasRoughness, v.hasSpecular, v.hasShininess, v.hasNormal, v.hasAO, v.hasEmissive, v.hasOpacity, v.hasHeight, v.hasDisplacement, v.hasReflection)
	#endif
	
	struct PBRSpecularGlossinessMaterial
	{
		//Texture indices
		int diffuseIdx;
		int specularIdx;
		int glossinessIdx;
		int normalIdx;
		int aoIdx;
		int emissiveIdx;
		int opacityIdx;
		int heightIdx;
		int displacementIdx;
		int reflectionIdx;

		//Presence flags (0 / 1)
		int hasDiffuse;
		int hasSpecular;
		int hasGlossiness;
		int hasNormal;
		int hasAO;
		int hasEmissive;
		int hasOpacity;
		int hasHeight;
		int hasDisplacement;
		int hasReflection;
	};
	#if defined(__cplusplus)
		SERIALISE(PBRSpecularGlossinessMaterial, v.diffuseIdx, v.specularIdx, v.glossinessIdx, v.normalIdx, v.aoIdx, v.emissiveIdx, v.opacityIdx, v.heightIdx, v.displacementIdx, v.reflectionIdx, v.hasDiffuse, v.hasSpecular, v.hasGlossiness, v.hasNormal, v.hasAO, v.hasEmissive, v.hasOpacity, v.hasHeight, v.hasDisplacement, v.hasReflection)
	#endif
	
	
	struct BlinnPhongMaterialNTC
	{
		int diffuseChannelR;
		int diffuseChannelG;
		int diffuseChannelB;
		int diffuseChannelA;
		int specularChannelR;
		int specularChannelG;
		int specularChannelB;
		int ambientChannelR;
		int ambientChannelG;
		int ambientChannelB;
		int emissiveChannelR;
		int emissiveChannelG;
		int emissiveChannelB;
		int normalChannelX;
		int normalChannelY;
		int normalChannelZ;
		int shininessChannel;
		int opacityChannel;
		int heightChannel;
		int displacementChannel;
		int lightmapChannelR;
		int lightmapChannelG;
		int lightmapChannelB;
		int reflectionChannelR;
		int reflectionChannelG;
		int reflectionChannelB;
			
		//Presence flags (0 / 1)
		int hasDiffuseChannelR;
		int hasDiffuseChannelG;
		int hasDiffuseChannelB;
		int hasDiffuseChannelA;
		int hasSpecularChannelR;
		int hasSpecularChannelG;
		int hasSpecularChannelB;
		int hasAmbientChannelR;
		int hasAmbientChannelG;
		int hasAmbientChannelB;
		int hasEmissiveChannelR;
		int hasEmissiveChannelG;
		int hasEmissiveChannelB;
		int hasNormalChannelX;
		int hasNormalChannelY;
		int hasNormalChannelZ;
		int hasShininessChannel;
		int hasOpacityChannel;
		int hasHeightChannel;
		int hasDisplacementChannel;
		int hasLightmapChannelR;
		int hasLightmapChannelG;
		int hasLightmapChannelB;
		int hasReflectionChannelR;
		int hasReflectionChannelG;
		int hasReflectionChannelB;
	};
	#if defined(__cplusplus)
		SERIALISE(BlinnPhongMaterialNTC, v.diffuseChannelR, v.diffuseChannelG, v.diffuseChannelB, v.diffuseChannelA, v.specularChannelR, v.specularChannelG, v.specularChannelB, v.ambientChannelR, v.ambientChannelG, v.ambientChannelB, v.emissiveChannelR, v.emissiveChannelG, v.emissiveChannelB, v.normalChannelX, v.normalChannelY, v.normalChannelZ, v.shininessChannel, v.opacityChannel, v.heightChannel, v.displacementChannel, v.lightmapChannelR, v.lightmapChannelG, v.lightmapChannelB, v.reflectionChannelR, v.reflectionChannelG, v.reflectionChannelB, v.hasDiffuseChannelR, v.hasDiffuseChannelG, v.hasDiffuseChannelB, v.hasDiffuseChannelA, v.hasSpecularChannelR, v.hasSpecularChannelG, v.hasSpecularChannelB, v.hasAmbientChannelR, v.hasAmbientChannelG, v.hasAmbientChannelB, v.hasEmissiveChannelR, v.hasEmissiveChannelG, v.hasEmissiveChannelB, v.hasNormalChannelX, v.hasNormalChannelY, v.hasNormalChannelZ, v.hasShininessChannel, v.hasOpacityChannel, v.hasHeightChannel, v.hasDisplacementChannel, v.hasLightmapChannelR, v.hasLightmapChannelG, v.hasLightmapChannelB, v.hasReflectionChannelR, v.hasReflectionChannelG, v.hasReflectionChannelB)
	#endif

	struct PBRMetallicRoughnessMaterialNTC
	{
		int baseColourChannelR;
		int baseColourChannelG;
		int baseColourChannelB;
		int baseColourChannelA;
		int metalnessChannel;
		int roughnessChannel;
		int specularChannelR;
		int specularChannelG;
		int specularChannelB;
		int shininessChannel;
		int normalChannelX;
		int normalChannelY;
		int normalChannelZ;
		int aoChannel;
		int emissiveChannelR;
		int emissiveChannelG;
		int emissiveChannelB;
		int opacityChannel;
		int heightChannel;
		int displacementChannel;
		int reflectionChannelR;
		int reflectionChannelG;
		int reflectionChannelB;

		//Presence flags (0 / 1)
		int hasBaseColourChannelR;
		int hasBaseColourChannelG;
		int hasBaseColourChannelB;
		int hasBaseColourChannelA;
		int hasMetalnessChannel;
		int hasRoughnessChannel;
		int hasSpecularChannelR;
		int hasSpecularChannelG;
		int hasSpecularChannelB;
		int hasShininessChannel;
		int hasNormalChannelX;
		int hasNormalChannelY;
		int hasNormalChannelZ;
		int hasAoChannel;
		int hasEmissiveChannelR;
		int hasEmissiveChannelG;
		int hasEmissiveChannelB;
		int hasOpacityChannel;
		int hasHeightChannel;
		int hasDisplacementChannel;
		int hasReflectionChannelR;
		int hasReflectionChannelG;
		int hasReflectionChannelB;

		std::uint32_t g0Channels;
		std::uint32_t g1Channels;
		std::uint32_t g0QuantLevels;
		std::uint32_t g1QuantLevels;
		std::uint32_t g0Resolution;
		std::uint32_t imageResolution;
		std::uint32_t numOctaves;
		std::uint32_t tileSize;
		std::uint32_t padding_align[2];
		std::uint32_t g0_offsets[4];
		std::uint32_t g1_offsets[4];
		std::uint32_t g0_resolutions[4];
		std::uint32_t g1_resolutions[4];
		std::uint32_t numLayers;
		std::uint32_t hiddenNeurons;
		std::uint32_t layer0_W_offset;
		std::uint32_t layer0_B_offset;
		std::uint32_t layer1_W_offset;
		std::uint32_t layer1_B_offset;
		std::uint32_t layer2_W_offset;
		std::uint32_t layer2_B_offset;
		std::uint32_t g0BufferIndex;
		std::uint32_t g1BufferIndex;
		std::uint32_t mlpBufferIndex;
	};
	#if defined(__cplusplus)
		SERIALISE(PBRMetallicRoughnessMaterialNTC, v.baseColourChannelR, v.baseColourChannelG, v.baseColourChannelB, v.baseColourChannelA, v.metalnessChannel, v.roughnessChannel, v.specularChannelR, v.specularChannelG, v.specularChannelB, v.shininessChannel, v.normalChannelX, v.normalChannelY, v.normalChannelZ, v.aoChannel, v.emissiveChannelR, v.emissiveChannelG, v.emissiveChannelB, v.opacityChannel, v.heightChannel, v.displacementChannel, v.reflectionChannelR, v.reflectionChannelG, v.reflectionChannelB, v.hasBaseColourChannelR, v.hasBaseColourChannelG, v.hasBaseColourChannelB, v.hasBaseColourChannelA, v.hasMetalnessChannel, v.hasRoughnessChannel, v.hasSpecularChannelR, v.hasSpecularChannelG, v.hasSpecularChannelB, v.hasShininessChannel, v.hasNormalChannelX, v.hasNormalChannelY, v.hasNormalChannelZ, v.hasAoChannel, v.hasEmissiveChannelR, v.hasEmissiveChannelG, v.hasEmissiveChannelB, v.hasOpacityChannel, v.hasHeightChannel, v.hasDisplacementChannel, v.hasReflectionChannelR, v.hasReflectionChannelG, v.hasReflectionChannelB, v.g0Channels, v.g1Channels, v.g0QuantLevels, v.g1QuantLevels, v.g0Resolution, v.imageResolution, v.numOctaves, v.tileSize, v.padding_align[0], v.g0_offsets[0], v.g0_offsets[1], v.g0_offsets[2], v.g0_offsets[3], v.g1_offsets[0], v.g1_offsets[1], v.g1_offsets[2], v.g1_offsets[3], v.g0_resolutions[0], v.g0_resolutions[1], v.g0_resolutions[2], v.g0_resolutions[3], v.g1_resolutions[0], v.g1_resolutions[1], v.g1_resolutions[2], v.g1_resolutions[3], v.numLayers, v.hiddenNeurons, v.layer0_W_offset, v.layer0_B_offset, v.layer1_W_offset, v.layer1_B_offset, v.layer2_W_offset, v.layer2_B_offset, v.g0BufferIndex, v.g1BufferIndex, v.mlpBufferIndex)
	#endif
	
	struct PBRSpecularGlossinessMaterialNTC
	{
		int diffuseChannelR;
		int diffuseChannelG;
		int diffuseChannelB;
		int diffuseChannelA;
		int specularChannelR;
		int specularChannelG;
		int specularChannelB;
		int glossinessChannel;
		int normalChannelX;
		int normalChannelY;
		int normalChannelZ;
		int aoChannel;
		int emissiveChannelR;
		int emissiveChannelG;
		int emissiveChannelB;
		int opacityChannel;
		int heightChannel;
		int displacementChannel;
		int reflectionChannelR;
		int reflectionChannelG;
		int reflectionChannelB;

		//Presence flags (0 / 1)
		int hasDiffuseChannelR;
		int hasDiffuseChannelG;
		int hasDiffuseChannelB;
		int hasDiffuseChannelA;
		int hasSpecularChannelR;
		int hasSpecularChannelG;
		int hasSpecularChannelB;
		int hasGlossinessChannel;
		int hasNormalChannelX;
		int hasNormalChannelY;
		int hasNormalChannelZ;
		int hasAoChannel;
		int hasEmissiveChannelR;
		int hasEmissiveChannelG;
		int hasEmissiveChannelB;
		int hasOpacityChannel;
		int hasHeightChannel;
		int hasDisplacementChannel;
		int hasReflectionChannelR;
		int hasReflectionChannelG;
		int hasReflectionChannelB;
	};
	#if defined(__cplusplus)
		SERIALISE(PBRSpecularGlossinessMaterialNTC, v.diffuseChannelR, v.diffuseChannelG, v.diffuseChannelB, v.diffuseChannelA, v.specularChannelR, v.specularChannelG, v.specularChannelB, v.glossinessChannel, v.normalChannelX, v.normalChannelY, v.normalChannelZ, v.aoChannel, v.emissiveChannelR, v.emissiveChannelG, v.emissiveChannelB, v.opacityChannel, v.heightChannel, v.displacementChannel, v.reflectionChannelR, v.reflectionChannelG, v.reflectionChannelB, v.hasDiffuseChannelR, v.hasDiffuseChannelG, v.hasDiffuseChannelB, v.hasDiffuseChannelA, v.hasSpecularChannelR, v.hasSpecularChannelG, v.hasSpecularChannelB, v.hasGlossinessChannel, v.hasNormalChannelX, v.hasNormalChannelY, v.hasNormalChannelZ, v.hasAoChannel, v.hasEmissiveChannelR, v.hasEmissiveChannelG, v.hasEmissiveChannelB, v.hasOpacityChannel, v.hasHeightChannel, v.hasDisplacementChannel, v.hasReflectionChannelR, v.hasReflectionChannelG, v.hasReflectionChannelB)
	#endif
	
}

#endif