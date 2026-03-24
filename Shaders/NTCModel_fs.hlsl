//VERY temp


#include <Types/ShaderMacros.hlsli>
#include <Types/Materials.h>

#pragma enable_dxc_extensions


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



[[vk::binding(0,0)]] Texture2D g_textures[] : register(t0, space1);
[[vk::binding(0,0)]] TextureCube g_cubemaps[] : register(t0, space2);
[[vk::binding(1,0)]] SamplerState g_samplers[] : register(s0, space0);
[[vk::binding(0,0)]] ConstantBuffer<NK::BlinnPhongMaterial> g_materials[] : register(b0, space0);
[[vk::binding(0,0)]] ByteAddressBuffer g0Buffer[] : register(t0, space0);
[[vk::binding(0,0)]] ByteAddressBuffer g1Buffer[] : register(t0, space0);
[[vk::binding(0,0)]] ByteAddressBuffer mlpBuffer[] : register(t0, space0);



PUSH_CONSTANTS_BLOCK(
	float4x4 modelMat;
	uint camDataBufferIndex;

	uint numLights;
	uint lightDataBufferIndex;

	uint skyboxCubemapIndex;
	uint irradianceCubemapIndex;
	uint prefilterCubemapIndex;
	uint brdfLUTIndex;
	//uint brdfLUTSamplerIndex;
	uint mlpBufferIndex;
	
	//VERY temp
	uint g0BufferIndex;
	uint g1BufferIndex;
	
	float maxIrradiance;
);



struct G0Features
{
	float channels[12];
};



struct G1Features
{
	float channels[10];
};


#define MAX_FEATURES 64 

float3 EvaluateMLP(G0Features g0, G1Features g1)
{
    float activations[MAX_FEATURES];
    for (uint init = 0; init < MAX_FEATURES; ++init) activations[init] = 0.0f;
    
    uint inputIdx = 0;
    for (uint i = 0; i < 12; ++i) activations[inputIdx++] = g0.channels[i];
    for (uint j = 0; j < 10; ++j) activations[inputIdx++] = g1.channels[j];

    uint byteOffset = 0;
    ByteAddressBuffer mlp = mlpBuffer[NonUniformResourceIndex(PC(mlpBufferIndex))];
    
    uint numLayers = mlp.Load(byteOffset); 
    byteOffset += 4;

    for (uint layer = 0; layer < numLayers; ++layer)
    {
        uint inF  = mlp.Load(byteOffset); byteOffset += 4;
        uint outF = mlp.Load(byteOffset); byteOffset += 4;

        float nextActivations[MAX_FEATURES];
        for(uint clr = 0; clr < MAX_FEATURES; ++clr) nextActivations[clr] = 0.0f;

        uint wIdx = 0;
        for (uint o = 0; o < outF; ++o)
        {
            float sum = 0.0f;
            for (uint i = 0; i < inF; ++i)
            {
                uint uintIndex = wIdx / 2;
                bool isUpper = (wIdx % 2) != 0;
                
                uint packedWeight = mlp.Load(byteOffset + (uintIndex * 4));
                
                float w = isUpper ? f16tof32(packedWeight >> 16) : f16tof32(packedWeight);
                
                sum += activations[i] * w;
                wIdx++;
            }
            nextActivations[o] = sum;
        }
        
        uint numWeightUints = (inF * outF + 1) / 2;
        byteOffset += numWeightUints * 4;

        for (uint b = 0; b < outF; ++b)
        {
            uint uintIndex = b / 2;
            bool isUpper = (b % 2) != 0;
            
            uint packedBias = mlp.Load(byteOffset + (uintIndex * 4));
            float bias = isUpper ? f16tof32(packedBias >> 16) : f16tof32(packedBias);

            float val = nextActivations[b] + bias;

            if (layer < numLayers - 1)
            {
                val = max(0.0f, val); 
            }
            
            activations[b] = val;
        }
        
        uint numBiasUints = (outF + 1) / 2;
        byteOffset += numBiasUints * 4;
    }

    return float3(activations[0], activations[1], activations[2]);
}


G1Features SampleG1(uint elementIndex)
{
	uint startBit = elementIndex * 40; 
	uint startWord = startBit / 32;
	uint bitInWord = startBit % 32;
	uint startByteIndex = startWord * 4;
	
	uint3 words = g1Buffer[NonUniformResourceIndex(PC(g1BufferIndex))].Load3(startByteIndex);
	
	uint lower32 = words.x >> bitInWord;
	uint upper8  = words.y >> bitInWord;
	
	if (bitInWord > 0)
	{
		lower32 |= (words.y << (32 - bitInWord));
		upper8  |= (words.z << (32 - bitInWord));
	}
	upper8 &= 0xFF;
	
	G1Features features;
	[unroll]
	for (uint i = 0; i < 10; ++i)
	{
		uint rawQuantisedValue;
		if (i < 8)
		{
			rawQuantisedValue = (lower32 >> (i * 4)) & 0xF;
		}
		else
		{
			rawQuantisedValue = (upper8 >> ((i - 8) * 4)) & 0xF;
		}
		
		features.channels[i] = (float)rawQuantisedValue / 15.0f;
	}
	
	return features;
}



G0Features SampleG0(uint elementIndex)
{
	uint startBit = elementIndex * 24; //24 bits per element
	uint startWord = startBit / 32;
	uint bitInWord = startBit % 32;
	uint startByteIndex = startWord * 4;
	
	uint2 words = g0Buffer[NonUniformResourceIndex(PC(g0BufferIndex))].Load2(startByteIndex);
	
	uint packedData = words.x >> bitInWord;
	if (bitInWord > 8)
	{
		packedData |= (words.y << (32 - bitInWord));
	}
	packedData &= 0xFFFFFF;
	
	G0Features features;
	[unroll]
	for (uint i = 0; i < 12; ++i)
	{
		uint bitShift = i * 2;
		uint rawQuantisedValue = (packedData >> bitShift) & 0x3;
		features.channels[i] = (float)rawQuantisedValue / 3.0f;
	}
	return features;
}



[shader("pixel")]
float4 FSMain(VertexOutput vertexOutput) : SV_TARGET
{
	float2 uv = vertexOutput.texCoord;
    
	uint g0_x = saturate(uv.x) * 2047;
	uint g0_y = saturate(uv.y) * 2047;
	uint g0_elementIndex = (g0_y * 2048) + g0_x;
	G0Features g0 = SampleG0(g0_elementIndex);
    
	uint g1_x = saturate(uv.x) * 1023;
	uint g1_y = saturate(uv.y) * 1023;
	uint g1_elementIndex = (g1_y * 1024) + g1_x;
	G1Features g1 = SampleG1(g1_elementIndex);

	float3 networkOutputRGB = EvaluateMLP(g0, g1);

	return float4(networkOutputRGB, 1.0f);
}