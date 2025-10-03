#include <Types/ShaderMacros.hlsli>

struct VertexOutput
{
	float4 pos : SV_POSITION;
	float2 texCoord : TEXCOORD_0;
};

[[vk::binding(0,0)]] Texture2D g_textures[] : register(t0, space0);
[[vk::binding(1,0)]] SamplerState g_samplers[] : register(s0, space0);

PUSH_CONSTANTS_BLOCK(
	uint textureIndex;
	uint samplerIndex;
);

float4 FSMain(VertexOutput vertexOutput) : SV_TARGET
{
    return g_textures[NonUniformResourceIndex(PC(textureIndex))].Sample(g_samplers[NonUniformResourceIndex(PC(samplerIndex))], vertexOutput.texCoord);
}