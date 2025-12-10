#include <Types/ShaderMacros.hlsli>
#include <Types/Materials.h>

#pragma enable_dxc_extensions


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
[[vk::binding(0,0)]] ConstantBuffer<NK::BlinnPhongMaterial> g_materials[] : register(b0, space0);


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

		float3 lightDirToFragPos;
		float attenuation = 1.0;

		//todo: replace with enums that match c++ side
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