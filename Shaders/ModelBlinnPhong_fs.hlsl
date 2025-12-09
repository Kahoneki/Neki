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

struct LightData
{		
	float4x4 viewProjMat; //For shadow mapping
	float3 colour;
	float intensity;
	float3 position;
	uint type;
	float3 direction;
	float innerAngle;
	float outerAngle;
	float constantAttenuation;
	float linearAttenuation;
	float quadraticAttenuation;
};

[[vk::binding(0,0)]] StructuredBuffer<LightData> g_lightData[] : register(t0, space0);
[[vk::binding(0,0)]] Texture2D g_textures[] : register(t0, space0);
[[vk::binding(1,0)]] SamplerState g_samplers[] : register(s0, space0);

PUSH_CONSTANTS_BLOCK(
	float4x4 modelMat;
	uint camDataBufferIndex;
	uint numLights;
	uint lightDataBufferIndex;
	uint shadowMapIndex;

	uint skyboxCubemapIndex;
	uint irradianceCubemapIndex;
	uint prefilterCubemapIndex;
	uint brdfLUTIndex;
	uint brdfLUTSamplerIndex;

	uint materialBufferIndex;
	uint samplerIndex;
);


float4 FSMain(VertexOutput vertexOutput) : SV_TARGET
{
	SamplerState linearSampler = g_samplers[NonUniformResourceIndex(PC(samplerIndex))];

	float shadowFactor = 1.0f;

	float3 lightDir = float3(0,-1,0);
	float nDotL = dot(vertexOutput.worldNormal, lightDir);
	if (nDotL > -1e-5)
	{
		shadowFactor = 0.0f;
	}
	else
	{
		LightData lightData = g_lightData[NonUniformResourceIndex(PC(lightDataBufferIndex))][0];
		float4 posInLightClipSpace = mul(lightData.viewProjMat, float4(vertexOutput.worldPos, 1.0));
		float3 posInLightNDC = posInLightClipSpace.xyz / posInLightClipSpace.w;
		float2 shadowMapUV = float2(posInLightNDC.x * 0.5f + 0.5f, posInLightNDC.y * -0.5f + 0.5f);
		if (shadowMapUV.x >= 0.0f && shadowMapUV.x <= 1.0f && shadowMapUV.y >= 0.0f && shadowMapUV.y <= 1.0f)
		{
			float shadow = g_textures[NonUniformResourceIndex(PC(shadowMapIndex))].Sample(linearSampler, shadowMapUV).r;
			float currentPixelDepthInLightSpace = posInLightNDC.z;
			float bias = 0.00001f; //Prevent shadow acne
			if (currentPixelDepthInLightSpace > shadow)
			{
				//Current pixel is further away than closest depth recorded in the map - hence it is in shadow
				shadowFactor = 0.0f;
			}
		}
	}

	return float4(1.0, 1.0, 1.0, 1.0) * shadowFactor;
}