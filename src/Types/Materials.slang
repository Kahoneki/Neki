struct BlinnPhongMaterial
{
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

struct PBRMetallicRoughnessMaterial
{
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

struct PBRSpecularGlossinessMaterial
{
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
};

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