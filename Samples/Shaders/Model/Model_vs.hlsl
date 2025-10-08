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
};


struct CamData
{
	float4x4 viewMat;
	float4x4 projMat;
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

	float4x4 mvp = mul(camData.projMat, mul(camData.viewMat, PC(modelMat)));
	output.pos = mul(mvp, float4(input.pos, 1.0));

	output.texCoord = input.texCoord;

    return output;
}