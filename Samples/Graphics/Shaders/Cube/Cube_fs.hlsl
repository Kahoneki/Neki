#include <Types/ShaderMacros.hlsli>

struct VertexOutput
{
	float4 pos : SV_POSITION;
	float3 colour : COLOR0;
};

float4 FSMain(VertexOutput vertexOutput) : SV_TARGET
{
    return float4(vertexOutput.colour, 1.0);
}