//VERY temp

static const float LIGHT_NEAR = 0.01f;
static const float LIGHT_FAR = 1000.0f;

struct VertexOutput
{
	float4 pos : SV_POSITION;
	float3 worldPos : WORLD_POS;
	float2 texCoord : TEXCOORD0;
	float3 fragPos : FRAG_POS;
	float3 worldNormal : WORLD_NORMAL;
	float3 camPos : CAM_POS;
	float3x3 TBN : TBN;
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

    uint numLayers;
    uint hiddenNeurons;
    uint frameIndex; //for stochastic filtering
};
[[vk::push_constant]] PushConstants pc;


float2 RandomFloat2(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.xx + p3.yz) * p3.zy); //[0,1]
}

float2 RandomGaussianFloat2(float2 p)
{
    float2 u = RandomFloat2(p);

    // Prevent log(0)
    u.x = max(u.x, 1e-6f);

    // Box-Muller transform
    const float TWO_PI = 6.28318530718f;
    float r = sqrt(-2.0f * log(u.x));
    float theta = TWO_PI * u.y;

    return float2(r * cos(theta), r * sin(theta));
}


half HardGELU(half x)
{
    return x * clamp(x / half(3.0) + half(0.5), half(0.0), half(1.0));
}


struct Material
{
    float3 colour;
    float3 normal;
    float ao;
    float displacement;
    float roughness;
};

static const uint MAX_G0_CHANNELS = 16;
struct G0Features
{
	float channels[MAX_G0_CHANNELS];
};

static const uint MAX_G1_CHANNELS = 20;
struct G1Features
{
	float channels[MAX_G1_CHANNELS];
};

G0Features SampleG0(int2 p)
{
	G0Features output;
	uint pixelIdx = p.y * pc.g0Resolution + p.x;

    // Calculate bytes per pixel based on channels and quant levels
    uint bitsPerChannel = (pc.g0QuantLevels <= 4) ? 2 : 4; // uniform across material - no warp divergence
    uint bitsPerPixel = pc.g0Channels * bitsPerChannel;
    uint bytesPerPixel = (bitsPerPixel + 7) / 8; //Round up to nearest byte

	uint byteOffset = pixelIdx * bytesPerPixel;
	uint alignedOffset = byteOffset & ~3;
	uint byteShift = (byteOffset & 3) * 8;

    // Load enough words to cover all channels
    uint numWords = (bytesPerPixel + 3) / 4 + 1;
    uint words[4]; //4 = max needed
    for (uint w = 0; w < min(numWords, 4); ++w)
    {
        words[w] = g0Buffer[pc.g0BufferIndex].Load(alignedOffset + (w * 4));
    }

    // Unpack channels
    float scale = 1.0 / float(pc.g0QuantLevels - 1);
    uint bitPos = byteShift;
    for (uint c = 0; c < pc.g0Channels; ++c)
    {
        uint wordIndex = bitPos / 32;
        uint bitOffsetInByte = (bitsPerChannel == 4) ? (4 - (bitPos % 8)) : (6 - (bitPos % 8));
        uint bitOffset = (bitPos / 8 % 4) * 8 + bitOffsetInByte;
        uint raw;
        if (bitsPerChannel == 2)
        {
            raw = (words[wordIndex] >> bitOffset) & 0x3;
        }
        else
        {
            raw = (words[wordIndex] >> bitOffset) & 0xF;
        }

        output.channels[c] = float(raw) * scale;
        bitPos += bitsPerChannel;
    }

    // Zero out unused channels
    for (uint c = pc.g0Channels; c < MAX_G0_CHANNELS; ++c)
    {
        output.channels[c] = 0.0f;
    }

    return output;
}

G1Features SampleG1(int2 p)
{
    G1Features output;
    uint pixelIdx = p.y * (pc.g0Resolution / 2) + p.x;

    // Calculate bytes per pixel based on channels and quant levels
    uint bitsPerChannel = (pc.g1QuantLevels <= 4) ? 2 : 4; // uniform across material - no warp divergence
    uint bitsPerPixel = pc.g1Channels * bitsPerChannel;
    uint bytesPerPixel = (bitsPerPixel + 7) / 8; // Round up to nearest byte

    uint byteOffset = pixelIdx * bytesPerPixel;
    uint alignedOffset = byteOffset & ~3;
    uint byteShift = (byteOffset & 3) * 8;

    // Load enough words to cover all channels
    uint numWords = (bytesPerPixel + 3) / 4 + 1;
    uint words[4]; // 4 = max needed
    for (uint w = 0; w < min(numWords, 4); ++w)
    {
        words[w] = g1Buffer[pc.g1BufferIndex].Load(alignedOffset + (w * 4));
    }

    // Unpack channels
    float scale = 1.0 / float(pc.g1QuantLevels - 1);
    uint bitPos = byteShift;
    for (uint c = 0; c < pc.g1Channels; ++c)
    {
        uint wordIndex = bitPos / 32;
        uint bitOffsetInByte = (bitsPerChannel == 4) ? (4 - (bitPos % 8)) : (6 - (bitPos % 8));
        uint bitOffset = (bitPos / 8 % 4) * 8 + bitOffsetInByte;
        uint raw;
        if (bitsPerChannel == 2)
        {
            raw = (words[wordIndex] >> bitOffset) & 0x3;
        }
        else
        {
            raw = (words[wordIndex] >> bitOffset) & 0xF;
        }

        output.channels[c] = float(raw) * scale;
        bitPos += bitsPerChannel;
    }

    // Zero out unused channels
    for (uint c = pc.g1Channels; c < MAX_G1_CHANNELS; ++c)
    {
        output.channels[c] = 0.0f;
    }

    return output;
}



Material EvaluateMLP(float2 uv)
{
    // Stochastic filtering
	float2 p = uv * pc.imageResolution;
    // float2 filterOffset = RandomGaussianFloat2(uv + pc.frameIndex) * 0.1f;
    // p += filterOffset;
    p = floor(p); // nearest neighbour
    p = clamp(p, float2(0.0f, 0.0f), float2(pc.imageResolution - 1, pc.imageResolution - 1));
	float2 local = fmod(p, pc.tileSize) / pc.tileSize;
    local = local * 2.0f - 1.0f;
    uv = p / pc.imageResolution;

    uint numInputs = (pc.numOctaves * 2 * 2) + (1) + (pc.g0Channels * 4) + (pc.g1Channels);
    static const uint MAX_INPUTS = 91;
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
    encodings[offset++] = half(0.0f); // normalised lod

    // G0
	int w = pc.g0Resolution;
	int h = pc.g0Resolution;
	p = int2(int(uv.x * (w-1)), int(uv.y * (h-1)));
	int x0 = clamp(int(floor(p.x)), 0, w-1);
	int x1 = clamp(x0+1, 0, w-1);
	int y0 = clamp(int(floor(p.y)), 0, h-1);
	int y1 = clamp(y0+1, 0, h-1);
	{
		G0Features s00 = SampleG0(int2(x0, y0));
		for (uint c=0; c<pc.g0Channels; ++c) { encodings[offset+c] = half(s00.channels[c]); }
        G0Features s01 = SampleG0(int2(x0, y1));
        for (uint c=0; c<pc.g0Channels; ++c) { encodings[offset+(1*pc.g0Channels)+c] = half(s01.channels[c]); }
        G0Features s10 = SampleG0(int2(x1, y0));
        for (uint c=0; c<pc.g0Channels; ++c) { encodings[offset+(2*pc.g0Channels)+c] = half(s10.channels[c]); }
        G0Features s11 = SampleG0(int2(x1, y1));
        for (uint c=0; c<pc.g0Channels; ++c) { encodings[offset+(3*pc.g0Channels)+c] = half(s11.channels[c]); }
    }
    offset += pc.g0Channels*4;

    // G1
	w = pc.g0Resolution/2;
	h = pc.g0Resolution/2;
	float fx = uv.x * (w - 1);
	float fy = uv.y * (h - 1);
	x0 = clamp(int(floor(fx)), 0, w-1);
	x1 = clamp(x0+1, 0, w-1);
	y0 = clamp(int(floor(fy)), 0, h-1);
	y1 = clamp(y0+1, 0, h-1);
	float2 weight = float2(fx-x0, fy-y0);
	{
		G1Features s00 = SampleG1(int2(x0, y0));
		G1Features s01 = SampleG1(int2(x0, y1));
		G1Features s10 = SampleG1(int2(x1, y0));
		G1Features s11 = SampleG1(int2(x1, y1));
		for (uint c=0; c<pc.g1Channels; ++c)
		{
			float latent = (1-weight.x) * (1-weight.y) * s00.channels[c];
			latent += (1-weight.x) * weight.y * s01.channels[c];
			latent += weight.x * (1-weight.y) * s10.channels[c];
			latent += weight.x * weight.y * s11.channels[c];
			encodings[offset+c] = half(latent);
        }
        offset += pc.g1Channels;
	}

    for (uint c = offset; c < MAX_INPUTS; ++c)
    {
        encodings[c] = half(0.0);
    }
	
	
	//MLP
	static const uint HIDDEN_DIM = 32;
	static const uint OUTPUT_DIM = 9;
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
    CoopVec<float16_t, OUTPUT_DIM> output = coopVecMatMulAdd<float16_t, OUTPUT_DIM, HIDDEN_DIM, float16_t>(
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

    Material material;
    material.colour = float3(output[0], output[1], output[2]);
    material.normal = float3(output[3], output[4], output[5]);
    material.ao = output[6];
    material.displacement = output[7];
    material.roughness = output[8];
    return material;
}

bool PointInPointLightShadow(float3 _fragPos, float3 _lightPos, uint _shadowIndex, SamplerState _samplerState)
{
    float3 lightToFrag = _fragPos - _lightPos;
    float3 absVec = abs(lightToFrag);
    float localZ = max(absVec.x, max(absVec.y, absVec.z));

    // Convert linear distance to non-linear depth (0..1) based on Perspective Projection
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
    // Intermediate vectors
    float3 H = normalize(_V + _L);
    float NDotL = max(dot(_N, _L), 0.0);
    float NDotV = max(dot(_N, _V), 0.0);
    float HDotV = max(dot(H, _V), 0.0);

    //----SPECULAR----//
    // DGF
    float D = DistributionGGX(_N, H, _roughness);
    float G = GeometrySmith(_N, _V, _L, _roughness);
    float3 F = FresnelSchlick(HDotV, _F0);

    // Cook-Torrance Specular BRDF
    float3 numerator = D * G * F;
    float denominator = max(4.0 * NDotV * NDotL, 0.0001); // Prevent divide by zero
    float3 specular = numerator / denominator;

    //----DIFFUSE----//
    float3 kS = F;                                  // specular power determined by fresnel
    float3 kD = 1.0 - kS;                           // gotta conserve that energy
    kD *= (1.0 - _metallic);                        // metals absorb all refracted light and convert to heat
    float3 diffuse = kD * _albedo / 3.141592653589; // todo: read this https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/

    float angleAttenuation = NDotL;
    return (diffuse + specular) * _radiance * NDotL;
}

float3 FresnelSchlickRoughness(float _cosTheta, float3 _F0, float _roughness)
{
    // For smooth surfaces, reflectivity at 90 degrees is 1.0 (pure white)
    // For rough surfaces, dampen the 1.0 based on roughness
    // I.e.: reflectivity increases as angle increases and decreases as roughness increases
    return _F0 + (max(1.0 - _roughness, _F0) - _F0) * pow(clamp(1.0 - _cosTheta, 0.0, 1.0), 5.0);
}

float3 CalculateIBL(float3 _albedo, float _roughness, float _metallic, float3 _F0, float3 _N, float3 _V)
{
    float NDotV = max(dot(_N, _V), 0.0);
    float3 R = reflect(-_V, _N);

    // Specular IBL (prefiltered environment map + brdf lut)
    const float MAX_REFLECTION_LOD = 4.0; // todo: use the actual number of mip levels
    float3 prefilteredColour = g_cubemaps[pc.prefilterCubemapIndex].SampleLevel(g_samplers[pc.samplerIndex], R, _roughness * MAX_REFLECTION_LOD).rgb;
    float2 brdfLUT = g_textures[pc.brdfLUTIndex].Sample(g_samplers[pc.brdfLUTSamplerIndex], float2(NDotV, _roughness)).rg;
    float3 specularIBL = prefilteredColour * (_F0 * brdfLUT.x + brdfLUT.y);

    // Energy conservation
    float3 kS = FresnelSchlickRoughness(max(dot(_N, _V), 0.0), _F0, _roughness);
    float3 kD = 1.0 - kS;
    kD *= (1.0 - _metallic);

    // Diffuse IBL (irradiance)
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
	Material material = EvaluateMLP(vertexOutput.texCoord);

    SamplerState linearSampler = g_samplers[pc.samplerIndex];
    StructuredBuffer<LightData> lightBuffer = g_lightData[pc.lightDataBufferIndex];

    float3 albedo = pow(material.colour, 2.2); // srgb -> linear
    float3 normal = normalize(mul(vertexOutput.TBN, material.normal * 2.0 - 1.0));
    float metallic = 1 - material.roughness;
    float roughness = material.roughness;
    float3 emissive = pow(float3(0,0,0), 2.2); // srgb -> linear

    float3 V = normalize(vertexOutput.camPos - vertexOutput.fragPos); // frag pos to camera

    // Base reflectance
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);

    // Direct lighting
    float3 totalDirectLighting = float3(0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < pc.numLights; ++i)
    {
        LightData light = lightBuffer[i];

        // Shadows - if point is in shadow from this light, just skip this light
        if (light.type == 2) // Point Light
        {
            if (PointInPointLightShadow(vertexOutput.worldPos, light.position, light.shadowMapIndex, linearSampler))
            {
                continue;
            }
        }
        else // Directional / Spot
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
                {
                    continue;
                }
            }
        }

        // Point is not in shadow from this light source - add up its contribution

        float3 lightDirToFragPos;
        float attenuation = 1.0;

        // todo: replace with enums that match c++ side
        if (light.type == 1) // Directional
        {
            lightDirToFragPos = normalize(-light.direction);
        }
        else // Point / Spot
        {
            float3 fragPosToLightPos = light.position - vertexOutput.worldPos;
            float dist = length(fragPosToLightPos);
            lightDirToFragPos = normalize(fragPosToLightPos);

            attenuation = 1.0 / (light.constantAttenuation + light.linearAttenuation * dist + light.quadraticAttenuation * dist * dist);

            if (light.type == 3) // Spot
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
    // if (material.hasAO)
    {
        ambientLighting *= material.ao;
    }

    float3 irradiance = g_cubemaps[pc.irradianceCubemapIndex].Sample(g_samplers[pc.samplerIndex], normal).rgb;
    // if (irradiance.r > 2.0f || irradiance.g > 2.0f || irradiance.b > 2.0f) { return float4(1.0f, 0.0f, 1.0f, 1.0f); }

    float3 colour = totalDirectLighting + ambientLighting + emissive;
    return float4(colour, 1.0);
}