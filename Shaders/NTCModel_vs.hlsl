#include <Types/ShaderMacros.hlsli>

#pragma enable_dxc_extensions

struct VertexInput
{
	ATTRIBUTE(float3, pos, NK::SHADER_ATTRIBUTE_LOCATION_POSITION, POSITION);
	ATTRIBUTE(float3, normal, NK::SHADER_ATTRIBUTE_LOCATION_NORMAL, NORMAL);
	ATTRIBUTE(float2, texCoord, NK::SHADER_ATTRIBUTE_LOCATION_TEXCOORD_0, TEXCOORD0);
	ATTRIBUTE(float4, tangent, NK::SHADER_ATTRIBUTE_LOCATION_TANGENT, TANGENT);
};

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

struct CamData
{
	float4x4 viewMat;
	float4x4 projMat;
	float4 pos;
};

[[vk::binding(0,0)]] ConstantBuffer<CamData> g_camData[] : register(b0, space0);

PUSH_CONSTANTS_BLOCK(
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
);

VertexOutput VSMain(VertexInput input, uint vertexID : SV_VertexID)
{
    VertexOutput output;
	CamData camData = g_camData[NonUniformResourceIndex(PC(camDataBufferIndex))];

    output.camPos = camData.pos.xyz;
	output.texCoord = input.texCoord;

	float4 worldPos = mul(PC(modelMat), float4(input.pos, 1.0));
	output.worldPos = worldPos.xyz;
	output.fragPos = float3(worldPos.xyz);
	output.pos = mul(camData.projMat, mul(camData.viewMat, worldPos));

	//TBN
    float sign = input.tangent.w;
	float3 T_os = input.tangent.xyz;
    float3 N_os = input.normal.xyz;
    float3 B_os = cross(T_os, N_os) * sign;
    
	float3 T_world = normalize( mul(PC(modelMat), float4(T_os, 0.0)).xyz );
	float3 B_world = normalize( mul(PC(modelMat), float4(B_os, 0.0)).xyz );
	float3 N_world = normalize( mul(PC(modelMat), float4(N_os, 0.0)).xyz );

	output.TBN_T = T_world;
	output.TBN_B = B_world;
	output.TBN_N = N_world;
	output.worldNormal = N_world;
	
    return output;
}