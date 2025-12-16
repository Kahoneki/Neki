#include <Types/ShaderMacros.hlsli>
#include <Types/Materials.h>

struct VertexOutput
{
	float4 pos : SV_POSITION;
	float3 texCoord : TEXCOORD0;
};

[[vk::binding(0,0)]] TextureCube g_skyboxes[] : register(t0, space1);
[[vk::binding(1,0)]] SamplerState g_samplers[] : register(s0, space0);

PUSH_CONSTANTS_BLOCK(
	float4x4 modelMat;
	uint camDataBufferIndex;

	uint numLights;
	uint lightDataBufferIndex;

	uint skyboxCubemapIndex;
	uint irradianceCubemapIndex;
	uint prefilterCubemapIndex;
	uint brdfLUTIndex;
	uint brdfLUTSamplerIndex;

	uint materialBufferIndex;
	uint samplerIndex;
	
	float time;
	float waveAmplitude;
);


[shader("pixel")]
float4 FSMain(VertexOutput vertexOutput) : SV_TARGET
{
	SamplerState sampler = g_samplers[NonUniformResourceIndex(PC(samplerIndex))];
	float4 skyboxSample = g_skyboxes[NonUniformResourceIndex(PC(skyboxCubemapIndex))].Sample(sampler, vertexOutput.texCoord);
	return skyboxSample;
}