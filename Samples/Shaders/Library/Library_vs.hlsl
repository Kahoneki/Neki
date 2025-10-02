#include <Types/ShaderAttribute.hlsli>

struct VertexInput
{
	ATTRIBUTE(float3, aPos, NK::SHADER_ATTRIBUTE_LOCATION_POSITION, SV_POSITION);
};

float4 VSMain(VertexInput vertex) : SV_POSITION
{
    return float4(vertex.aPos, 1.0f);
}