#include <Types/ShaderMacros.hlsli>

struct VertexOutput
{
	float4 pos : SV_POSITION;
};


PUSH_CONSTANTS_BLOCK(
	float4x4 modelMat;
	uint numLights;
	uint lightCamDataBufferIndex;
);


[shader("pixel")]
void FSMain(VertexOutput vertexOutput)
{
}