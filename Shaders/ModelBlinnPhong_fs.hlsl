#include <Types/ShaderMacros.hlsli>
#include <Types/Materials.h>

#pragma enable_dxc_extensions


static const float LIGHT_NEAR = 0.01f;
static const float LIGHT_FAR = 1000.0f;


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
	float4x4 viewProjMat;
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
	uint shadowMapIndex;
};


[[vk::binding(0,0)]] StructuredBuffer<LightData> g_lightData[] : register(t0, space0);
[[vk::binding(0,0)]] Texture2D g_textures[] : register(t0, space0);
[[vk::binding(0,0)]] TextureCube g_cubemaps[] : register(t0, space1);
[[vk::binding(1,0)]] SamplerState g_samplers[] : register(s0, space0);
[[vk::binding(0,0)]] ConstantBuffer<NK::BlinnPhongMaterial> g_materials[] : register(b0, space0);


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
);



bool PointInPointLightShadow(float3 _fragPos, float3 _lightPos, uint _shadowIndex, SamplerState _samplerState)
{
	float3 lightToFrag = _fragPos - _lightPos;
	float3 absVec = abs(lightToFrag);
	float localZ = max(absVec.x, max(absVec.y, absVec.z));
	
	//Convert linear distance to non-linear depth (0..1) based on perspective projection
	float normDepth = (LIGHT_FAR / (LIGHT_FAR - LIGHT_NEAR)) - ((LIGHT_FAR * LIGHT_NEAR) / (LIGHT_FAR - LIGHT_NEAR)) / localZ;

	float closestDepth = g_cubemaps[NonUniformResourceIndex(_shadowIndex)].Sample(_samplerState, lightToFrag).r;

	float bias = 0.0005f;
	return (normDepth - bias) > closestDepth;
}



[shader("pixel")]
float4 FSMain(VertexOutput vertexOutput) : SV_TARGET
{
	NK::BlinnPhongMaterial material = g_materials[NonUniformResourceIndex(PC(materialBufferIndex))];
	SamplerState linearSampler = g_samplers[NonUniformResourceIndex(PC(samplerIndex))];
	StructuredBuffer<LightData> lightBuffer = g_lightData[NonUniformResourceIndex(PC(lightDataBufferIndex))];


	//Textures
	float4 diffuseSample = float4(1.0f, 1.0f, 1.0f, 1.0f);
	if (material.hasDiffuse)
	{
		diffuseSample = g_textures[NonUniformResourceIndex(material.diffuseIdx)].Sample(linearSampler, vertexOutput.texCoord);
	}

	float3 specularSample = float3(1.0f, 1.0f, 1.0f);
	if (material.hasSpecular)
	{
		specularSample = g_textures[NonUniformResourceIndex(material.specularIdx)].Sample(linearSampler, vertexOutput.texCoord).rgb;
	}

	float3 normal = normalize(vertexOutput.worldNormal);
	if (material.hasNormal)
	{
		float3 normalMap = g_textures[NonUniformResourceIndex(material.normalIdx)].Sample(linearSampler, vertexOutput.texCoord).rgb;
		normal = normalize(normalMap * 2.0f - 1.0f);
		normal = normalize(mul(vertexOutput.TBN, normal)); 
	}

	float shininess = 32.0f;
	if (material.hasShininess)
	{
		const float MAX_SHININESS = 255.0f;
		shininess = g_textures[NonUniformResourceIndex(material.shininessIdx)].Sample(linearSampler, vertexOutput.texCoord).r * MAX_SHININESS;
	}

	float3 emissive = float3(0.0f, 0.0f, 0.0f);
	if (material.hasEmissive)
	{
		emissive = g_textures[NonUniformResourceIndex(material.emissiveIdx)].Sample(linearSampler, vertexOutput.texCoord).rgb;
	}

	float3 viewDir = normalize(vertexOutput.camPos - vertexOutput.worldPos);


	//Lighting
	float3 diffuse = float3(0.0f, 0.0f, 0.0f);
	float3 specular = float3(0.0f, 0.0f, 0.0f);

	for (uint i = 0; i < PC(numLights); ++i)
	{
		LightData light = lightBuffer[i];

		//Shadows - if point is in shadow from this light, just skip this light
		if (light.type == 2) //Point Light
		{
float3 lightToFrag = vertexOutput.worldPos - light.position;
float debugDepth = g_cubemaps[NonUniformResourceIndex(light.shadowMapIndex)].Sample(linearSampler, float3(-1,0,0)).r;
return float4(debugDepth, debugDepth, debugDepth, 1.0f);

			if (PointInPointLightShadow(vertexOutput.worldPos, light.position, light.shadowMapIndex, linearSampler))
			{
				return float4(1.0, 0.0, 1.0, 1.0);
			}
		}
		else //Directional / Spot
		{
			float4 posInLightClipSpace = mul(light.viewProjMat, float4(vertexOutput.worldPos, 1.0));
			float3 posInLightNDC = posInLightClipSpace.xyz / posInLightClipSpace.w;
			float2 shadowMapUV = float2(posInLightNDC.x * 0.5f + 0.5f, posInLightNDC.y * -0.5f + 0.5f);

			if (shadowMapUV.x >= 0.0f && shadowMapUV.x <= 1.0f && shadowMapUV.y >= 0.0f && shadowMapUV.y <= 1.0f && posInLightNDC.z <= 1.0f)
			{
				float shadowDepth = g_textures[NonUniformResourceIndex(light.shadowMapIndex)].Sample(linearSampler, shadowMapUV).r;

				float currentPixelDepth = posInLightNDC.z;
				
				float bias = 0.0005f; 
				if ((currentPixelDepth - bias) > shadowDepth)
				{
					continue;
				}
			}
			else
			{
				continue;
			}
		}


		//Point is not in shadow from this light source - add up its contribution

		float3 lightDirToFragPos;
		float attenuation = 1.0;

		if (light.type == 1) //Directional
		{
			lightDirToFragPos = normalize(-light.direction);
		}
		else //Point / Spot
		{
			float3 fragPosToLightPos = light.position - vertexOutput.worldPos;
			float dist = length(fragPosToLightPos);
			lightDirToFragPos = normalize(fragPosToLightPos);

			attenuation = 1.0 / (light.constantAttenuation + light.linearAttenuation * dist + light.quadraticAttenuation * dist * dist);

			if (light.type == 3) //Spot
			{
				float theta = dot(lightDirToFragPos, normalize(-light.direction));
				float innerCos = cos(light.innerAngle);
				float outerCos = cos(light.outerAngle);
				float epsilon = innerCos - outerCos;
				float intensity = clamp((theta - outerCos) / epsilon, 0.0f, 1.0f);
				attenuation *= intensity;
			}
		}

		if (attenuation > 0.0001)
		{
			float NdotL = dot(normal, lightDirToFragPos);
			if (NdotL > 0.0f)
			{
				//Lambertian diffuse
				diffuse += NdotL * light.colour * light.intensity * attenuation;

				//Blinn-phong specular
				float3 halfwayDir = normalize(lightDirToFragPos + viewDir);
				float spec = pow(max(dot(normal, halfwayDir), 0.0f), shininess);
				specular += spec * specularSample * light.colour * light.intensity * attenuation;
			}
		}
	}


	//Ambient
	float3 ambient = float3(0.05, 0.05, 0.05);
	if (material.hasAmbient)
	{
		ambient *= g_textures[NonUniformResourceIndex(material.ambientIdx)].Sample(linearSampler, vertexOutput.texCoord).rgb;
	}
	ambient *= diffuseSample.rgb;


	float3 result = ambient + (diffuse * diffuseSample.rgb) + specular + emissive;
	return float4(result, 1.0f);
}