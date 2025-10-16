#ifndef NEKI_MATERIALS_H
#define NEKI_MATERIALS_H


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

		//Presence flags
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
	
}

#endif