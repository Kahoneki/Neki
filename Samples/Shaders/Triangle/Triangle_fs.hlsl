#include <Types/ShaderAttribute.hlsli>

struct VertexOutput
{
	ATTRIBUTE(float4, pos, NK::SHADER_ATTRIBUTE_LOCATION_POSITION, SV_POSITION);
	ATTRIBUTE(float3, colour, NK::SHADER_ATTRIBUTE_LOCATION_COLOUR_0, COLOR0);
};

float4 FSMain(VertexOutput vertexOutput) : SV_TARGET
{
    return float4(float3(vertexOutput.colour), 1.0f);
}