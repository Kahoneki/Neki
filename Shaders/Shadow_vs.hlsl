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


struct LightData
{		
	float4x4 viewProjMat; //For shadow mapping
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
};

[[vk::binding(0,0)]] ConstantBuffer<LightCamData> g_lightCamData[] : register(b0, space0);


PUSH_CONSTANTS_BLOCK(
	float4x4 modelMat;
	uint numLights;
	uint lightCamDataBufferIndex;
);


VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
	LightCamData lightCamData = g_lightCamData[NonUniformResourceIndex(PC(lightCamDataBufferIndex))];
	
    output.pos = mul(lightCamData.projMat, mul(lightCamData.viewMat, mul(PC(modelMat), float4(input.pos, 1.0))));

    return output;
}