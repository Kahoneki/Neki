#include <Types/ShaderMacros.hlsli>

struct VertexOutput
{
	float4 pos : SV_POSITION;
};

float4 FSMain(VertexOutput vertexOutput) : SV_TARGET
{
	return float4(1.0, 0.0, 0.0, 1.0);
}