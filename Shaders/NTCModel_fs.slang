#include "Types/Materials.slang"

//shut up the compiler (slang gives explicit binding overlap warnings, neki uses bindless descriptors so overlapping is intentional)
#pragma warning(disable: 39001)

static const float LIGHT_NEAR = 0.01f;
static const float LIGHT_FAR = 1000.0f;

static const uint MAX_INPUTS = 96;
static const uint HIDDEN_DIM = 16;
static const uint MAX_OUTPUTS = 16;

static const uint FeatureLevelLookup[11] = { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 3 };

struct VertexOutput
{
	float4 pos : SV_POSITION;
    float3 worldPos : WORLD_POS;
	float2 texCoord : TEXCOORD0;
	float3 fragPos : FRAG_POS;
	float3 worldNormal : WORLD_NORMAL;
	float3 camPos : CAM_POS;
    float3 TBN_T : TBN_T;
    float3 TBN_B : TBN_B;
    float3 TBN_N : TBN_N;
};

struct LightData
{
    float4x4 viewProjMat;
    float3 colour;
    float intensity;
    float3 position;
    uint type;
    float3 direction;
    float innerAngle;
    float outerAngle;
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
    uint shadowMapIndex;
    float3 padding;
};

[[vk::binding(0,0)]] ByteAddressBuffer g0Buffer[] : register(t0, space0);
[[vk::binding(0,0)]] ByteAddressBuffer g1Buffer[] : register(t0, space0);
[[vk::binding(0, 0)]] ByteAddressBuffer mlpBuffer[] : register(t0, space0);
[[vk::binding(0, 0)]] TextureCube g_cubemaps[] : register(t0, space2);
[[vk::binding(1, 0)]] SamplerState g_samplers[] : register(s0, space0);
[[vk::binding(0, 0)]] StructuredBuffer<LightData> g_lightData[] : register(t0, space0);
[[vk::binding(0, 0)]] Texture2D g_textures[] : register(t0, space1);
[[vk::binding(0,0)]] ConstantBuffer<PBRMetallicRoughnessMaterialNTC> g_materials[] : register(b0, space0);

struct PushConstants
{
	float4x4 modelMat;
	uint camDataBufferIndex;

	uint numLights;
	uint lightDataBufferIndex;

	uint skyboxCubemapIndex;
	uint irradianceCubemapIndex;
	uint prefilterCubemapIndex;
	uint brdfLUTIndex;
	uint brdfLUTSamplerIndex;

	uint materialBufferIndex;
	uint samplerIndex;
	
	float maxIrradiance;
	
	uint g0BufferIndex;
	uint g1BufferIndex;
	uint mlpBufferIndex;
	uint layer0_W_offset;
    uint layer0_B_offset;
    uint layer1_W_offset;
    uint layer1_B_offset;
    uint layer2_W_offset;
    uint layer2_B_offset;

    uint g0Channels;
    uint g1Channels;
    uint g0QuantLevels;
    uint g1QuantLevels;

    uint g0Resolution;
    uint imageResolution;
    uint numOctaves;
    uint tileSize;
    
    uint g0_offsets[4];
    uint g1_offsets[4];
    uint g0_resolutions[4];
    uint g1_resolutions[4];

    uint numLayers;
    uint hiddenNeurons;
    uint frameIndex; //for stochastic filtering
};
[[vk::push_constant]] PushConstants pc;

uint pcg_hash(uint seed) 
{
    uint state = seed * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float random(inout uint seed) 
{
    seed = pcg_hash(seed);
    return float(seed) / 4294967296.0;
}

float CalculateLevelOfDetail(float2 uv, float2 textureRes)
{
    float2 dx = ddx(uv * textureRes);
    float2 dy = ddy(uv * textureRes);
    float dxx = dot(dx, dx);
    float dyy = dot(dy, dy);
    float rho = max(dxx, dyy);
    float lod = 0.5 * log2(rho);
    return max(0.0, lod);
}

half HardGELU(half x)
{
    return x * clamp(x / half(3.0) + half(0.5), half(0.0), half(1.0));
}

static const uint MAX_G0_CHANNELS = 16;
struct G0Features { float channels[MAX_G0_CHANNELS]; };

static const uint MAX_G1_CHANNELS = 20;
struct G1Features { float channels[MAX_G1_CHANNELS]; };

G0Features SampleG0(int2 p, uint fl)
{
	G0Features output;

    uint g0ResAtLevel = pc.g0_resolutions[fl];
	uint pixelIdx = p.y * g0ResAtLevel + p.x;

    uint bitsPerChannel = (pc.g0QuantLevels <= 4) ? 2 : 4;
    uint bitsPerPixel = pc.g0Channels * bitsPerChannel;
    uint bytesPerPixel = (bitsPerPixel + 7) / 8;

	uint byteOffset = pc.g0_offsets[fl] + (pixelIdx * bytesPerPixel);
	uint alignedOffset = byteOffset & ~3;
	uint byteShift = (byteOffset & 3) * 8;

    uint numWords = (bytesPerPixel + 3) / 4 + 1;
    uint words[4];
    for (uint w = 0; w < min(numWords, 4); ++w)
    {
        words[w] = g0Buffer[pc.g0BufferIndex].Load(alignedOffset + (w * 4));
    }

    float scale = 1.0 / float(pc.g0QuantLevels - 1);
    uint bitPos = byteShift;
    for (uint c = 0; c < pc.g0Channels; ++c)
    {
        uint wordIndex = bitPos / 32;
        uint bitOffsetInByte = (bitsPerChannel == 4) ? (4 - (bitPos % 8)) : (6 - (bitPos % 8));
        uint bitOffset = (bitPos / 8 % 4) * 8 + bitOffsetInByte;
        uint raw;
        if (bitsPerChannel == 2) { raw = (words[wordIndex] >> bitOffset) & 0x3; }
        else { raw = (words[wordIndex] >> bitOffset) & 0xF; }

        output.channels[c] = float(raw) * scale;
        bitPos += bitsPerChannel;
    }

    for (uint c = pc.g0Channels; c < MAX_G0_CHANNELS; ++c)
        output.channels[c] = 0.0f;

    return output;
}

G1Features SampleG1(int2 p, uint fl)
{
    G1Features output;

    uint g1ResAtLevel = pc.g1_resolutions[fl];
    uint pixelIdx = p.y * g1ResAtLevel + p.x; 

    uint bitsPerChannel = (pc.g1QuantLevels <= 4) ? 2 : 4;
    uint bitsPerPixel = pc.g1Channels * bitsPerChannel;
    uint bytesPerPixel = (bitsPerPixel + 7) / 8; 

    uint byteOffset = pc.g1_offsets[fl] + (pixelIdx * bytesPerPixel);
    uint alignedOffset = byteOffset & ~3;
    uint byteShift = (byteOffset & 3) * 8;

    uint numWords = (bytesPerPixel + 3) / 4 + 1;
    uint words[4]; 
    for (uint w = 0; w < min(numWords, 4); ++w)
    {
        words[w] = g1Buffer[pc.g1BufferIndex].Load(alignedOffset + (w * 4));
    }

    float scale = 1.0 / float(pc.g1QuantLevels - 1);
    uint bitPos = byteShift;
    for (uint c = 0; c < pc.g1Channels; ++c)
    {
        uint wordIndex = bitPos / 32;
        uint bitOffsetInByte = (bitsPerChannel == 4) ? (4 - (bitPos % 8)) : (6 - (bitPos % 8));
        uint bitOffset = (bitPos / 8 % 4) * 8 + bitOffsetInByte;
        uint raw;
        if (bitsPerChannel == 2) { raw = (words[wordIndex] >> bitOffset) & 0x3; }
        else { raw = (words[wordIndex] >> bitOffset) & 0xF; }

        output.channels[c] = float(raw) * scale;
        bitPos += bitsPerChannel;
    }

    for (uint c = pc.g1Channels; c < MAX_G1_CHANNELS; ++c)
        output.channels[c] = 0.0f;

    return output;
}

CoopVec<float16_t, MAX_OUTPUTS> EvaluateMLP(float2 uv, float lod)
{
    uv = frac(uv);
    
    uint numMips = uint(log2(float(pc.imageResolution))) - 1;
    
    // Nearest-neighbor mip selection fallback. (Use DLSS / TAA if you want stochastic filtering)
    uint mip = min(uint(round(lod)), numMips - 1);
    uint featureLevel = FeatureLevelLookup[min(mip, 10u)];

    float normalizedLod = saturate(float(mip) / float(max(1u, numMips - 1)));

    float2 p = uv * float(pc.imageResolution);
    p = clamp(p, float2(0.0f, 0.0f), float2(pc.imageResolution - 1, pc.imageResolution - 1));
    float2 local = fmod(p, float(pc.tileSize)) / float(pc.tileSize);
    local = local * 2.0f - 1.0f;

    uint numInputs = (pc.numOctaves * 2 * 2) + (1) + (pc.g0Channels * 4) + (pc.g1Channels);
    CoopVec<float16_t, MAX_INPUTS> encodings;

    uint offset = 0;
    for (uint i=0; i<pc.numOctaves; ++i)
    {
        float scale = exp2(float(i)) * 3.141592653589;
        encodings[i*4] = half(sin(scale * local.x));
        encodings[i*4+1] = half(sin(scale * local.y));
        encodings[i*4+2] = half(cos(scale * local.x));
        encodings[i*4+3] = half(cos(scale * local.y));
        offset += 4;
    }
    encodings[offset++] = half(normalizedLod);

    // G0
    int w0 = pc.g0_resolutions[featureLevel];
    int h0 = w0;
    float2 p0 = float2(uv.x * (w0 - 1.0f), uv.y * (h0 - 1.0f));
    int x0_0 = clamp(int(floor(p0.x)), 0, w0-1);
    int x1_0 = clamp(x0_0+1, 0, w0-1);
    int y0_0 = clamp(int(floor(p0.y)), 0, h0-1);
    int y1_0 = clamp(y0_0+1, 0, h0-1);
    {
        G0Features s00_0 = SampleG0(int2(x0_0, y0_0), featureLevel);
        for (uint c=0; c<pc.g0Channels; ++c) { encodings[offset+c] = half(s00_0.channels[c]); }
        
        G0Features s01_0 = SampleG0(int2(x0_0, y1_0), featureLevel);
        for (uint c=0; c<pc.g0Channels; ++c) { encodings[offset+(1*pc.g0Channels)+c] = half(s01_0.channels[c]); }
        
        G0Features s10_0 = SampleG0(int2(x1_0, y0_0), featureLevel);
        for (uint c=0; c<pc.g0Channels; ++c) { encodings[offset+(2*pc.g0Channels)+c] = half(s10_0.channels[c]); }
        
        G0Features s11_0 = SampleG0(int2(x1_0, y1_0), featureLevel);
        for (uint c=0; c<pc.g0Channels; ++c) { encodings[offset+(3*pc.g0Channels)+c] = half(s11_0.channels[c]); }
    }
    offset += pc.g0Channels * 4;

    // G1
    int w1 = pc.g1_resolutions[featureLevel];
    int h1 = w1;
    float fx = uv.x * (w1 - 1.0f);
    float fy = uv.y * (h1 - 1.0f);
    int x0_1 = clamp(int(floor(fx)), 0, w1-1);
    int x1_1 = clamp(x0_1+1, 0, w1-1);
    int y0_1 = clamp(int(floor(fy)), 0, h1-1);
    int y1_1 = clamp(y0_1+1, 0, h1-1);
    float2 weight = float2(fx-float(x0_1), fy-float(y0_1));
    {
        G1Features s00_1 = SampleG1(int2(x0_1, y0_1), featureLevel);
        G1Features s01_1 = SampleG1(int2(x0_1, y1_1), featureLevel);
        G1Features s10_1 = SampleG1(int2(x1_1, y0_1), featureLevel);
        G1Features s11_1 = SampleG1(int2(x1_1, y1_1), featureLevel);
        
        for (uint c=0; c<pc.g1Channels; ++c)
        {
            float latent = (1.0f-weight.x) * (1.0f-weight.y) * s00_1.channels[c];
            latent += (1.0f-weight.x) * weight.y * s01_1.channels[c];
            latent += weight.x * (1.0f-weight.y) * s10_1.channels[c];
            latent += weight.x * weight.y * s11_1.channels[c];
            encodings[offset+c] = half(latent);
        }
    }
    offset += pc.g1Channels;

    for (uint c = offset; c < MAX_INPUTS; ++c)
    {
        encodings[c] = half(0.0);
    }
    
    //MLP
    CoopVec<float16_t, HIDDEN_DIM> hidden0 = coopVecMatMulAdd<float16_t, HIDDEN_DIM, MAX_INPUTS, float16_t>(
        encodings,
        CoopVecComponentType.Float16,
        mlpBuffer[pc.mlpBufferIndex],
        pc.layer0_W_offset,
        CoopVecComponentType.Float16,
        mlpBuffer[pc.mlpBufferIndex],
        pc.layer0_B_offset,
        CoopVecComponentType.Float16,
        CoopVecMatrixLayout.InferencingOptimal,
        false,
        0
    );
    [ForceUnroll]
    for (int i = 0; i < HIDDEN_DIM; i++)
    {
        hidden0[i] = hidden0[i] * min(max(hidden0[i] / half(3.0) + half(0.5), half(0.0)), half(1.0));
    }

    CoopVec<float16_t, HIDDEN_DIM> hidden1;
    if (pc.numLayers >= 3)
    {
        hidden1 = coopVecMatMulAdd<float16_t, HIDDEN_DIM, HIDDEN_DIM, float16_t>(
            hidden0,
            CoopVecComponentType.Float16,
            mlpBuffer[pc.mlpBufferIndex],
            pc.layer1_W_offset,
            CoopVecComponentType.Float16,
            mlpBuffer[pc.mlpBufferIndex],
            pc.layer1_B_offset,
            CoopVecComponentType.Float16,
            CoopVecMatrixLayout.InferencingOptimal,
            false,
            0
        );
        [ForceUnroll]
        for (int i = 0; i < HIDDEN_DIM; i++)
        {
            hidden1[i] = hidden1[i] * min(max(hidden1[i] / half(3.0) + half(0.5), half(0.0)), half(1.0));
        }
    }
    else
    {
        hidden1 = hidden0;
    }

    uint out_W_offset = (pc.numLayers >= 3) ? pc.layer2_W_offset : pc.layer1_W_offset;
    uint out_B_offset = (pc.numLayers >= 3) ? pc.layer2_B_offset : pc.layer1_B_offset;
    CoopVec<float16_t, MAX_OUTPUTS> output = coopVecMatMulAdd<float16_t, MAX_OUTPUTS, HIDDEN_DIM, float16_t>(
        hidden1,
        CoopVecComponentType.Float16,
        mlpBuffer[pc.mlpBufferIndex],
        out_W_offset,
        CoopVecComponentType.Float16,
        mlpBuffer[pc.mlpBufferIndex],
        out_B_offset,
        CoopVecComponentType.Float16,
        CoopVecMatrixLayout.InferencingOptimal,
        false,
        0
    );

    return output;
}

bool PointInPointLightShadow(float3 _fragPos, float3 _lightPos, uint _shadowIndex, SamplerState _samplerState)
{
    float3 lightToFrag = _fragPos - _lightPos;
    float3 absVec = abs(lightToFrag);
    float localZ = max(absVec.x, max(absVec.y, absVec.z));

    float normDepth = (LIGHT_FAR / (LIGHT_FAR - LIGHT_NEAR)) - ((LIGHT_FAR * LIGHT_NEAR) / (LIGHT_FAR - LIGHT_NEAR)) / localZ;
    float closestDepth = g_cubemaps[NonUniformResourceIndex(_shadowIndex)].Sample(_samplerState, lightToFrag).r;

    float bias = 0.00002f;
    return (normDepth - bias) > closestDepth;
}

float DistributionGGX(float3 _normal, float3 _halfway, float _roughness)
{
    float a = _roughness * _roughness;
    float a2 = a * a;
    float nDotH = max(dot(_normal, _halfway), 0.0);
    float nDotH2 = nDotH * nDotH;

    float numerator = a2;
    float denominator = (nDotH2 * (a2 - 1.0) + 1.0);
    denominator = max(3.141592653589 * denominator * denominator, 0.001);

    return numerator / denominator;
}

float GeometrySchlickGGX(float _NDotV, float _roughness)
{
    float r = _roughness + 1.0;
    float k = (r * r) / 8.0;

    float numerator = _NDotV;
    float denominator = _NDotV * (1.0 - k) + k;

    return numerator / denominator;
}

float GeometrySmith(float3 _N, float3 _V, float3 _L, float _roughness)
{
    float nDotV = max(dot(_N, _V), 0.0);
    float nDotL = max(dot(_N, _L), 0.0);
    float ggx2 = GeometrySchlickGGX(nDotV, _roughness);
    float ggx1 = GeometrySchlickGGX(nDotL, _roughness);
    return ggx1 * ggx2;
}

float3 FresnelSchlick(float _cosTheta, float3 _F0)
{
    return _F0 + (1.0 - _F0) * pow(clamp(1.0 - _cosTheta, 0.0, 1.0), 5.0);
}

float3 CalculateDirectLight(float3 _albedo, float _metallic, float _roughness, float3 _F0, float3 _N, float3 _V, float3 _L, float3 _radiance)
{
    float3 H = normalize(_V + _L);
    float NDotL = max(dot(_N, _L), 0.0);
    float NDotV = max(dot(_N, _V), 0.0);
    float HDotV = max(dot(H, _V), 0.0);

    float D = DistributionGGX(_N, H, _roughness);
    float G = GeometrySmith(_N, _V, _L, _roughness);
    float3 F = FresnelSchlick(HDotV, _F0);

    float3 numerator = D * G * F;
    float denominator = max(4.0 * NDotV * NDotL, 0.0001); 
    float3 specular = numerator / denominator;

    float3 kS = F;                                  
    float3 kD = 1.0 - kS;                           
    kD *= (1.0 - _metallic);                        
    float3 diffuse = kD * _albedo / 3.141592653589; 

    return (diffuse + specular) * _radiance * NDotL;
}

float3 FresnelSchlickRoughness(float _cosTheta, float3 _F0, float _roughness)
{
    return _F0 + (max(1.0 - _roughness, _F0) - _F0) * pow(clamp(1.0 - _cosTheta, 0.0, 1.0), 5.0);
}

float3 CalculateIBL(float3 _albedo, float _roughness, float _metallic, float3 _F0, float3 _N, float3 _V)
{
    float NDotV = max(dot(_N, _V), 0.0);
    float3 R = reflect(-_V, _N);

    const float MAX_REFLECTION_LOD = 10.0; 
    float3 prefilteredColour = g_cubemaps[pc.prefilterCubemapIndex].SampleLevel(g_samplers[pc.samplerIndex], R, _roughness * MAX_REFLECTION_LOD).rgb;
    float2 brdfLUT = g_textures[pc.brdfLUTIndex].Sample(g_samplers[pc.brdfLUTSamplerIndex], float2(NDotV, _roughness)).rg;
    float3 specularIBL = prefilteredColour * (_F0 * brdfLUT.x + brdfLUT.y);

    float3 kS = FresnelSchlickRoughness(max(dot(_N, _V), 0.0), _F0, _roughness);
    float3 kD = 1.0 - kS;
    kD *= (1.0 - _metallic);

    float3 irradiance = g_cubemaps[pc.irradianceCubemapIndex].Sample(g_samplers[pc.samplerIndex], _N).rgb;
    if (!all(isfinite(irradiance)))
    {
        irradiance = float3(0, 0, 0);
    }
    irradiance = min(irradiance, pc.maxIrradiance);
    float3 diffuseIBL = irradiance * _albedo * kD;

    return diffuseIBL + specularIBL;
}

[shader("pixel")]
float4 main(VertexOutput vertexOutput) : SV_TARGET
{
    //Stochastic filtering
    uint seed = uint(vertexOutput.pos.x) + uint(vertexOutput.pos.y) * 8192 + pc.frameIndex * 16777216;
    float baseLod = CalculateLevelOfDetail(vertexOutput.texCoord, float2(pc.imageResolution, pc.imageResolution));
    float lodRandom = random(seed);
    uint mip = uint(max(0.0f, floor(baseLod + lodRandom)));
    uint numMips = uint(max(0, int(log2(float(pc.imageResolution))) - 1));
    mip = min(mip, numMips - 1);
    float mipRes = float(pc.imageResolution >> mip);
    float2 uvTexels = vertexOutput.texCoord * mipRes - 0.5f;
    float2 uvRandom = float2(random(seed), random(seed));
    float2 chosenTexel = floor(uvTexels + uvRandom);

    //Snap to nearest trained texel center
    float2 snappedUV = (chosenTexel + 0.5f) / mipRes;
    CoopVec<float16_t, MAX_OUTPUTS> mlpOutput = EvaluateMLP(snappedUV, float(mip));
    
    PBRMetallicRoughnessMaterialNTC ntcMat = g_materials[pc.materialBufferIndex];
    SamplerState linearSampler = g_samplers[pc.samplerIndex];
    StructuredBuffer<LightData> lightBuffer = g_lightData[pc.lightDataBufferIndex];

    float3 albedoSample = float3(0,0,0);
    if (bool(ntcMat.hasBaseColourChannelR)) { albedoSample.x = mlpOutput[ntcMat.baseColourChannelR]; }
    if (bool(ntcMat.hasBaseColourChannelG)) { albedoSample.y = mlpOutput[ntcMat.baseColourChannelG]; }
    if (bool(ntcMat.hasBaseColourChannelB)) { albedoSample.z = mlpOutput[ntcMat.baseColourChannelB]; }

    float3 normalSample = float3(0,0,0);
    if (bool(ntcMat.hasNormalChannelX)) { normalSample.x = mlpOutput[ntcMat.normalChannelX]; }
    if (bool(ntcMat.hasNormalChannelY)) { normalSample.y = mlpOutput[ntcMat.normalChannelY]; }
    if (bool(ntcMat.hasNormalChannelZ)) { normalSample.z = mlpOutput[ntcMat.normalChannelZ]; }
    
    float3 emissiveSample = float3(0,0,0);
    if (bool(ntcMat.hasEmissiveChannelR)) { emissiveSample.x = mlpOutput[ntcMat.emissiveChannelR]; }
    if (bool(ntcMat.hasEmissiveChannelG)) { emissiveSample.y = mlpOutput[ntcMat.emissiveChannelG]; }
    if (bool(ntcMat.hasEmissiveChannelB)) { emissiveSample.z = mlpOutput[ntcMat.emissiveChannelB]; }

    float metallicSample = 0.0f;
    if (bool(ntcMat.hasMetalnessChannel)) { metallicSample = mlpOutput[ntcMat.metalnessChannel]; }

    float roughnessSample = 0.0f;
    if (bool(ntcMat.hasRoughnessChannel)) { roughnessSample = mlpOutput[ntcMat.roughnessChannel]; }

    float aoSample = 1.0f;
    if (bool(ntcMat.hasAoChannel)) { aoSample = mlpOutput[ntcMat.aoChannel]; }

    float3x3 TBN = transpose(float3x3(vertexOutput.TBN_T, vertexOutput.TBN_B, vertexOutput.TBN_N));

    float3 albedo = pow(saturate(albedoSample.rgb), 2.2);
    float3 normal = normalize(mul(TBN, saturate(normalSample.rgb) * 2.0 - 1.0));
    float3 emissive = pow(saturate(emissiveSample.rgb), 2.2);
    float metallic = saturate(metallicSample);
    float roughness = saturate(roughnessSample);
    float ao = saturate(aoSample);
    
    //Specular speckles fix
    float3 dndx = ddx(normal);
    float3 dndy = ddy(normal);
    float variance = 0.5f * (dot(dndx, dndx) + dot(dndy, dndy));
    float roughnessAA = sqrt(roughness * roughness + variance * 3.0f); 
    roughness = clamp(roughnessAA, 0.04f, 1.0f);
    
    float3 V = normalize(vertexOutput.camPos - vertexOutput.fragPos); 

    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);

    float3 totalDirectLighting = float3(0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < pc.numLights; ++i)
    {
        LightData light = lightBuffer[i];

        if (light.type == 2) 
        {
            if (PointInPointLightShadow(vertexOutput.worldPos, light.position, light.shadowMapIndex, linearSampler))
                continue;
        }
        else 
        {
            float4 posInLightClipSpace = mul(light.viewProjMat, float4(vertexOutput.worldPos, 1.0));
            float3 posInLightNDC = posInLightClipSpace.xyz / posInLightClipSpace.w;
            float2 shadowMapUV = float2(posInLightNDC.x * 0.5f + 0.5f, posInLightNDC.y * -0.5f + 0.5f);

            if (shadowMapUV.x >= 0.0f && shadowMapUV.x <= 1.0f && shadowMapUV.y >= 0.0f && shadowMapUV.y <= 1.0f && posInLightNDC.z <= 1.0f)
            {
                float shadowDepth = g_textures[NonUniformResourceIndex(light.shadowMapIndex)].Sample(linearSampler, shadowMapUV).r;
                float currentPixelDepth = posInLightNDC.z;
                
                float bias = 0.0005f; 
                if ((currentPixelDepth - bias) > shadowDepth)
                    continue;
            }
        }

        float3 lightDirToFragPos;
        float attenuation = 1.0;

        if (light.type == 1) 
        {
            lightDirToFragPos = normalize(-light.direction);
        }
        else 
        {
            float3 fragPosToLightPos = light.position - vertexOutput.worldPos;
            float dist = length(fragPosToLightPos);
            lightDirToFragPos = normalize(fragPosToLightPos);

            attenuation = 1.0 / (light.constantAttenuation + light.linearAttenuation * dist + light.quadraticAttenuation * dist * dist);

            if (light.type == 3) 
            {
                float theta = dot(lightDirToFragPos, normalize(-light.direction));
                float innerCos = cos(light.innerAngle);
                float outerCos = cos(light.outerAngle);
                float epsilon = innerCos - outerCos;
                float intensity = clamp((theta - outerCos) / epsilon, 0.0f, 1.0f);
                attenuation *= intensity;
            }
        }

        if (attenuation > 0.0001)
        {
            float NdotL = dot(normal, lightDirToFragPos);
            if (NdotL > 0.0f)
            {
                float3 radiance = light.colour * light.intensity;
                float3 directLighting = CalculateDirectLight(albedo, metallic, roughness, F0, normal, V, lightDirToFragPos, radiance);
                totalDirectLighting += directLighting * attenuation;
            }
        }
    }

    float3 ambientLighting = CalculateIBL(albedo, roughness, metallic, F0, normal, V);
    ambientLighting *= ao;
    
    float3 colour = totalDirectLighting + ambientLighting + emissive;

    return float4(colour, 1.0);
}