#include <Types/ShaderMacros.hlsli>

struct VertexOutput
{
	float4 pos : SV_POSITION;
	float3 worldPos : WORLD_POS;
	float2 texCoord : TEXCOORD0;
	float3 fragPos : FRAG_POS;
	float3 worldNormal : WORLD_NORMAL;
	float3 camPos : CAM_POS;
	float3x3 TBN : TBN;	
};

struct LightCamData
{
	float4x4 viewMat;
	float4x4 projMat;
};

[[vk::binding(0,0)]] ConstantBuffer<LightCamData> g_lightCamData[] : register(b0, space0);
[[vk::binding(0,0)]] Texture2D g_textures[] : register(t0, space0);
[[vk::binding(1,0)]] SamplerState g_samplers[] : register(s0, space0);

PUSH_CONSTANTS_BLOCK(
	float4x4 modelMat;
	uint camDataBufferIndex;
	uint lightCamDataBufferIndex;
	uint shadowMapIndex;
	uint skyboxCubemapIndex;
	uint materialBufferIndex;
	uint samplerIndex;
);


float4 FSMain(VertexOutput vertexOutput) : SV_TARGET
{
	SamplerState linearSampler = g_samplers[NonUniformResourceIndex(PC(samplerIndex))];

	float shadowFactor = 1.0f;
	LightCamData lightCamData = g_lightCamData[NonUniformResourceIndex(PC(lightCamDataBufferIndex))];
	float4 posInLightClipSpace = mul(lightCamData.projMat, mul(lightCamData.viewMat, float4(vertexOutput.worldPos, 1.0)));
	float3 posInLightNDC = posInLightClipSpace.xyz / posInLightClipSpace.w;
	float2 shadowMapUV = float2(posInLightNDC.x * 0.5f + 0.5f, posInLightNDC.y * -0.5f + 0.5f);
	if (shadowMapUV.x >= 0.0f && shadowMapUV.x <= 1.0f && shadowMapUV.y >= 0.0f && shadowMapUV.y <= 1.0f)
	{
		float shadow = g_textures[NonUniformResourceIndex(PC(shadowMapIndex))].Sample(linearSampler, shadowMapUV).r;
		float currentPixelDepthInLightSpace = posInLightNDC.z;
		float bias = 0.05f; //Prevent shadow acne
		if (currentPixelDepthInLightSpace > shadow)
		{
			//Current pixel is further away than closest depth recorded in the map - hence it is in shadow
			shadowFactor = 0.0f;
		}
	}
	return float4(1.0, 1.0, 1.0, 1.0) * shadowFactor;
}