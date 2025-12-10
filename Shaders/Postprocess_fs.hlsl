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



//Credit: https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
float3 ACESFilm(float3 _x)
{
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
	return saturate((_x * (a * _x + b)) / (_x * (c * _x + d) + e));
}



[shader("pixel")]
float4 FSMain(VertexOutput vertexOutput) : SV_TARGET
{
	SamplerState linearSampler = g_samplers[NonUniformResourceIndex(PC(samplerIndex))];
	float3 sceneColour = g_textures[NonUniformResourceIndex(PC(sceneColourIndex))].Sample(linearSampler, vertexOutput.texCoord).rgb;
	
	float exposure = 0.6f;
	sceneColour *= exposure;
	
	sceneColour = ACESFilm(sceneColour);

	return float4(sceneColour, 1.0);
}