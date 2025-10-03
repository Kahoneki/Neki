#include <Types/ShaderMacros.hlsli>

struct VertexOutput
{
	ATTRIBUTE(float4, pos, NK::SHADER_ATTRIBUTE_LOCATION_POSITION, SV_POSITION);
};

PUSH_CONSTANTS_BLOCK(
	float textureIndex;
);

float4 FSMain(VertexOutput vertexOutput) : SV_TARGET
{
    return float4(PC(textureIndex), 0.0f, 0.0f, 1.0f);
}