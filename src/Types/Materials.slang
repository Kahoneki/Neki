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

    uint g0Channels;
    uint g1Channels;
    uint g0QuantLevels;
    uint g1QuantLevels;
    uint g0Resolution;
    uint imageResolution;
    uint numOctaves;
    uint tileSize;
    uint4 g0_offsets;
    uint4 g1_offsets;
    uint4 g0_resolutions;
    uint4 g1_resolutions;
    uint numLayers;
    uint hiddenNeurons;
    uint layer0_W_offset;
    uint layer0_B_offset;
    uint layer1_W_offset;
    uint layer1_B_offset;
    uint layer2_W_offset;
    uint layer2_B_offset;
    uint g0BufferIndex;
    uint g1BufferIndex;
    uint mlpBufferIndex;
};