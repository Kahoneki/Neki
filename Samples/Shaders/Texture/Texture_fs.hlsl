#include <Types/ShaderAttribute.hlsli>

struct VertexOutput
{
	ATTRIBUTE(float4, pos, NK::SHADER_ATTRIBUTE_LOCATION_POSITION, SV_POSITION);
};

float4 FSMain(VertexOutput vertexOutput) : SV_TARGET
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}