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

	float4 sceneColour = g_textures[NonUniformResourceIndex(PC(sceneColourIndex))].Sample(linearSampler, vertexOutput.texCoord);
	float sceneDepth = g_textures[NonUniformResourceIndex(PC(sceneDepthIndex))].Sample(linearSampler, vertexOutput.texCoord);
	float shadowMap = g_textures[NonUniformResourceIndex(PC(shadowMapIndex))].Sample(linearSampler, vertexOutput.texCoord);
	// if (shadowMap > 0.9999999)
	// {
	// 	return float4(0.0,0.0,0.0,1.0);
	// }
    //return float4(shadowMap, shadowMap, shadowMap, 1.0);
	return sceneColour;
}