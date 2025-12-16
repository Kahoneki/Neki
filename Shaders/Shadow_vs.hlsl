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


PUSH_CONSTANTS_BLOCK(
	float4x4 viewProjMat;
	float4x4 modelMat;
	float time;
	float waveAmplitude;
);


VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
	float offset = sin(PC(time) * 2.0f + input.pos.y * 5.0f) * PC(waveAmplitude);
	float3 manipulatedPos = input.pos + (input.normal * offset);
    output.pos = mul(PC(viewProjMat), mul(PC(modelMat), float4(manipulatedPos, 1.0)));
    return output;
}