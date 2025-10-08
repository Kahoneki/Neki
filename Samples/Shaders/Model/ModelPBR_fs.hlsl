#include <Types/ShaderMacros.hlsli>
#include <Types/Materials.h>

struct VertexOutput
{
	float4 pos : SV_POSITION;
	float2 texCoord : TEXCOORD0;
};

[[vk::binding(0,0)]] Texture2D g_textures[] : register(t0, space0);
[[vk::binding(1,0)]] SamplerState g_samplers[] : register(s0, space0);
[[vk::binding(0,0)]] ConstantBuffer<NK::PBRMaterial> g_materials[] : register(b0, space0);

PUSH_CONSTANTS_BLOCK(
	float4x4 modelMat;
	uint camDataBufferIndex;
	uint materialBufferIndex;
	uint samplerIndex;
);

float4 FSMain(VertexOutput vertexOutput) : SV_TARGET
{
	NK::PBRMaterial material = g_materials[NonUniformResourceIndex(PC(materialBufferIndex))];

	//From here you can access material.hasX to see if the material has a texture of type X, and if it does, access material.XIdx to get the index into g_textures for the texture of type X

	SamplerState sampler = g_samplers[NonUniformResourceIndex(PC(samplerIndex))];
	if (material.hasBaseColour == 1)
	{
		Texture2D baseColourTexture = g_textures[NonUniformResourceIndex(material.baseColourIdx)];
		return baseColourTexture.Sample(sampler, vertexOutput.texCoord);
	}
	else
	{
		return float4(0.0,1.0,0.0,0.0);
	}
}