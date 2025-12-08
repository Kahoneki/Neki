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
	float3x3 TBN : TBN;	
	float3 bitangent : BITANGENT;
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
	uint shadowMapIndex;
	uint skyboxCubemapIndex;
	uint materialBufferIndex;
	uint samplerIndex;
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

	float3 T_os = float3(input.tangent.x, input.tangent.yz);
    float  sign = input.tangent.w;
    float3 N_os = float3(input.normal.x, input.normal.yz);
	//T_os = normalize(T_os - N_os * dot(N_os, T_os));
    
    float3 B_os = cross(T_os, N_os) * sign;
    
    float3 T = normalize( mul(PC(modelMat), float4(T_os, 0.0)).xyz );
    float3 B = normalize( mul(PC(modelMat), float4(B_os, 0.0)).xyz );
    float3 N = normalize( mul(PC(modelMat), float4(N_os, 0.0)).xyz );

    output.TBN = transpose(float3x3(T, B, N));
    output.bitangent = B;

	output.worldNormal = N;

    return output;
}