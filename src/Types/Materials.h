#ifndef NEKI_MATERIALS_H
#define NEKI_MATERIALS_H

#if defined(__cplusplus)
	#include <Core/Utils/Serialisation.h>
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

	struct PBRMaterial
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
		SERIALISE(PBRMaterial, v.baseColourIdx, v.metalnessIdx, v.roughnessIdx, v.specularIdx, v.shininessIdx, v.normalIdx, v.aoIdx, v.emissiveIdx, v.opacityIdx, v.heightIdx, v.displacementIdx, v.reflectionIdx, v.hasBaseColour, v.hasMetalness, v.hasRoughness, v.hasSpecular, v.hasShininess, v.hasNormal, v.hasAO, v.hasEmissive, v.hasOpacity, v.hasHeight, v.hasDisplacement, v.hasReflection)
	#endif
	
}

#endif