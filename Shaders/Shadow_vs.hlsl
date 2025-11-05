#include <Types/ShaderMacros.hlsli>


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
};


struct LightCamData
{
	float4x4 viewMat;
	float4x4 projMat;
};

[[vk::binding(0,0)]] ConstantBuffer<LightCamData> g_lightCamData[] : register(b0, space0);


PUSH_CONSTANTS_BLOCK(
	float4x4 modelMat;
	uint lightCamDataBufferIndex;
);


VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
	LightCamData lightCamData = g_lightCamData[NonUniformResourceIndex(PC(lightCamDataBufferIndex))];
	
    output.pos = mul(lightCamData.projMat, mul(lightCamData.viewMat, mul(PC(modelMat), float4(input.pos, 1.0))));

    return output;
}