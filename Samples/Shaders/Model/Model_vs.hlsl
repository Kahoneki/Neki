#include <Types/ShaderMacros.hlsli>


struct VertexInput
{
	ATTRIBUTE(float3, pos, NK::SHADER_ATTRIBUTE_LOCATION_POSITION, POSITION);
	ATTRIBUTE(float3, normal, NK::SHADER_ATTRIBUTE_LOCATION_NORMAL, NORMAL);
	ATTRIBUTE(float2, texCoord, NK::SHADER_ATTRIBUTE_LOCATION_TEXCOORD_0, TEXCOORD0);
	ATTRIBUTE(float3, tangent, NK::SHADER_ATTRIBUTE_LOCATION_TANGENT, TANGENT);
	ATTRIBUTE(float3, bitangent, NK::SHADER_ATTRIBUTE_LOCATION_BITANGENT, BITANGENT);
};

struct VertexOutput
{
	float4 pos : SV_POSITION;
	float2 texCoord : TEXCOORD0;
	float3 fragPos : FRAG_POS;
	float3 worldNormal : WORLD_NORMAL;
	float3 camPos : CAM_POS;
	float3x3 TBN : TBN;	
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
	uint materialBufferIndex;
	uint samplerIndex;
);


VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
	CamData camData = g_camData[NonUniformResourceIndex(PC(camDataBufferIndex))];

	output.texCoord = input.texCoord;
	
	float4 worldPos = mul(PC(modelMat), float4(input.pos, 1.0));
	output.fragPos = float3(worldPos.xyz);
	output.pos = mul(camData.projMat, mul(camData.viewMat, worldPos));

	float3 T = normalize(float3(mul(PC(modelMat), float4(input.tangent, 1.0)).xyz));
	float3 B = normalize(float3(mul(PC(modelMat), float4(input.bitangent, 1.0)).xyz));
	float3 N = normalize(float3(mul(PC(modelMat), float4(input.normal, 1.0)).xyz));
	output.TBN = float3x3(T,B,N);
	
	output.worldNormal = N;

    return output;
}