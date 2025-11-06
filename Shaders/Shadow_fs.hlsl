#include <Types/ShaderMacros.hlsli>

struct VertexOutput
{
	float4 pos : SV_POSITION;
};


PUSH_CONSTANTS_BLOCK(
	float4x4 modelMat;
	uint lightCamDataBufferIndex;
);


[shader("pixel")]
float4 FSMain(VertexOutput vertexOutput) : SV_TARGET
{
    return float4(0, 0, 0, 0);
}