#include <Types/ShaderMacros.hlsli>

struct VertexOutput
{
	float4 pos : SV_POSITION;
	float2 texCoord : TEXCOORD;
};

[[vk::binding(0,0)]] Texture2D g_textures[] : register(t0, space0);
[[vk::binding(1,0)]] SamplerState g_samplers[] : register(s0, space0);

PUSH_CONSTANTS_BLOCK(
	uint shadowMapIndex;
	uint sceneColourIndex;
	uint sceneDepthIndex;
	uint samplerIndex;
);


[shader("pixel")]
float4 FSMain(VertexOutput vertexOutput) : SV_TARGET
{
	SamplerState linearSampler = g_samplers[NonUniformResourceIndex(PC(samplerIndex))];
    return g_textures[NonUniformResourceIndex(PC(sceneColourIndex))].Sample(linearSampler, vertexOutput.texCoord);
}