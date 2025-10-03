#include <Types/ShaderMacros.hlsli>

struct VertexOutput
{
	float4 pos : SV_POSITION;
	float2 texCoord : TEXCOORD_0;
};

[[vk::binding(0,0)]] Texture2D g_textures[] : register(t0, space0);

PUSH_CONSTANTS_BLOCK(
	float textureIndex;
);

float4 FSMain(VertexOutput vertexOutput) : SV_TARGET
{
    return float4(vertexOutput.texCoord, 0.0f, 1.0f);
}