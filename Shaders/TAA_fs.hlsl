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
	float4x4 inverseViewProj;
	float4x4 prevViewProj;
);


[shader("pixel")]
float4 FSMain(VertexOutput vertexOutput) : SV_TARGET
{
	SamplerState s = g_samplers[NonUniformResourceIndex(PC(samplerIndex))];
	float2 uv = vertexOutput.texCoord;
	
	//Get current frame and depth
	uint w, h;
	g_textures[NonUniformResourceIndex(PC(sceneColourIndex))].GetDimensions(w,h);
	int3 texelCoord = int3(uv * float2(w, h), 0);
	float3 currentColour = g_textures[NonUniformResourceIndex(PC(sceneColourIndex))].Load(texelCoord).rgb;
	float depth = g_textures[NonUniformResourceIndex(PC(sceneDepthIndex))].Load(texelCoord).r;
	
	//Reprojection (get last frame's uv)
	float4 ndc = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0);
	float4 worldPos = mul(PC(inverseViewProj), ndc);
	worldPos /= worldPos.w;
	
	float4 prevClip = mul(PC(prevViewProj), worldPos);
	float2 prevNDC = prevClip.xy / prevClip.w;
	float2 prevUV = float2(prevNDC.x * 0.5 + 0.5, 0.5 - prevNDC.y * 0.5);
	
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
	
	//Read history and clamp
	float3 historyColour = g_textures[NonUniformResourceIndex(PC(historyColourIndex))].SampleLevel(s, prevUV, 0).rgb;
	historyColour = clamp(historyColour, minColour, maxColour);
	
	//10% current, 90% history
	float blendWeight = 0.1;
	
	//Reset the history if UV goes off screen
	if (prevUV.x < 0.0 || prevUV.x > 1.0 || prevUV.y < 0.0 || prevUV.y > 1.0)
	{
		blendWeight = 1.0; //100% current, 0% history
	}
	
	float3 finalColour = lerp(historyColour, currentColour, blendWeight);
	
	return float4(finalColour, 1.0);
}