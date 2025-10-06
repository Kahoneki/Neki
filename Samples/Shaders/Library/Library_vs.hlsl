#include <Types/ShaderMacros.hlsli>

struct VertexInput
{
	//None for library sample
};

float4 VSMain(VertexInput vertex) : SV_POSITION
{
    return float4(0.0f, 0.0f, 0.0f, 0.0f);
}