#include <Types/ShaderMacros.hlsli>

struct VertexOutput
{
	float4 pos : SV_POSITION;
	float2 texCoord : TEXCOORD;
};

[[vk::binding(0,0)]] Texture2D g_textures[] : register(t0, space0);
[[vk::binding(1,0)]] SamplerState g_samplers[] : register(s0, space0);

PUSH_CONSTANTS_BLOCK(
	uint sceneColourIndex;
	uint sceneDepthIndex;
	uint historyColourIndex;
	uint samplerIndex;
	uint velocityTextureIndex;
);

[shader("pixel")]
float4 FSMain(VertexOutput vertexOutput) : SV_TARGET
{
	SamplerState s = g_samplers[NonUniformResourceIndex(PC(samplerIndex))];
	float2 uv = vertexOutput.texCoord;
	
	uint w, h;
	g_textures[NonUniformResourceIndex(PC(sceneColourIndex))].GetDimensions(w,h);
	int3 texelCoord = int3(uv * float2(w, h), 0);
	
	float3 currentColour = g_textures[NonUniformResourceIndex(PC(sceneColourIndex))].Load(texelCoord).rgb;
	
	float2 velocity = g_textures[NonUniformResourceIndex(PC(velocityTextureIndex))].Load(texelCoord).rg;
	float2 prevUV = uv - velocity;
	
	float2 texelSize = 1.0 / float2(w,h);
	float3 minColour = currentColour;
	float3 maxColour = currentColour;
	
	int2 offsets[8] = {
		int2(-1, -1), int2(0, -1), int2(1, -1),
		int2(-1,  0),              int2(1,  0),
		int2(-1,  1), int2(0,  1), int2(1,  1)
	};
	
	for (int i = 0; i < 8; ++i)
	{
		int3 neighborCoord = texelCoord + int3(offsets[i], 0);
		neighborCoord.x = clamp(neighborCoord.x, 0, w - 1);
		neighborCoord.y = clamp(neighborCoord.y, 0, h - 1);
		float3 neighbor = g_textures[NonUniformResourceIndex(PC(sceneColourIndex))].Load(neighborCoord).rgb;
		minColour = min(minColour, neighbor);
		maxColour = max(maxColour, neighbor);
	}
	
	float3 historyColour = g_textures[NonUniformResourceIndex(PC(historyColourIndex))].SampleLevel(s, prevUV, 0).rgb;
	historyColour = clamp(historyColour, minColour, maxColour);
	
	float blendWeight = 0.1;
	if (prevUV.x < 0.0 || prevUV.x > 1.0 || prevUV.y < 0.0 || prevUV.y > 1.0)
	{
		blendWeight = 1.0;
	}
	
	return float4(lerp(historyColour, currentColour, blendWeight), 1.0);
}