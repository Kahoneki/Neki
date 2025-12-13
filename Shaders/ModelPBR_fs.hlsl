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
	float3 bitangent : BITANGENT;
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
[[vk::binding(0,0)]] Texture2D g_textures[] : register(t0, space1);
[[vk::binding(0,0)]] TextureCube g_cubemaps[] : register(t0, space2);
[[vk::binding(1,0)]] SamplerState g_samplers[] : register(s0, space0);
[[vk::binding(0,0)]] ConstantBuffer<NK::PBRMaterial> g_materials[] : register(b0, space0);


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
	
	//Convert linear distance to non-linear depth (0..1) based on Perspective Projection
	float normDepth = (LIGHT_FAR / (LIGHT_FAR - LIGHT_NEAR)) - ((LIGHT_FAR * LIGHT_NEAR) / (LIGHT_FAR - LIGHT_NEAR)) / localZ;

	float closestDepth = g_cubemaps[NonUniformResourceIndex(_shadowIndex)].Sample(_samplerState, lightToFrag).r;

	float bias = 0.0005f;
	return (normDepth - bias) > closestDepth;
}



float DistributionGGX(float3 _normal, float3 _halfway, float _roughness)
{
    float a = _roughness*_roughness;
    float a2 = a*a;
    float nDotH = max(dot(_normal, _halfway), 0.0);
    float nDotH2 = nDotH * nDotH;

    float numerator = a2;
    float denominator = (nDotH2 * (a2 - 1.0) + 1.0);
    denominator = max(3.141592653589 * denominator * denominator, 0.001);

    return numerator / denominator;
}



float GeometrySchlickGGX(float _NDotV, float _roughness)
{
    float r = _roughness + 1.0;
    float k = (r*r) / 8.0;

    float numerator = _NDotV;
    float denominator = _NDotV * (1.0 - k) + k;

    return numerator / denominator;
}



float GeometrySmith(float3 _N, float3 _V, float3 _L, float _roughness)
{
    float nDotV = max(dot(_N, _V), 0.0);
    float nDotL = max(dot(_N, _L), 0.0);
    float ggx2 = GeometrySchlickGGX(nDotV, _roughness);
    float ggx1 = GeometrySchlickGGX(nDotL, _roughness);
    return ggx1 * ggx2;
}



float3 FresnelSchlick(float _cosTheta, float3 _F0)
{
    return _F0 + (1.0 - _F0) * pow(clamp(1.0 - _cosTheta, 0.0, 1.0), 5.0);
}



float3 CalculateDirectLight(float3 _albedo, float _metallic, float _roughness, float3 _F0, float3 _N, float3 _V, float3 _L, float3 _radiance)
{
	//Intermediate vectors
    float3 H = normalize(_V + _L);
    float NDotL = max(dot(_N, _L), 0.0);
    float NDotV = max(dot(_N, _V), 0.0);
    float HDotV = max(dot(H, _V), 0.0);


	//----SPECULAR----//
	//DGF
	float D = DistributionGGX(_N, H, _roughness);
	float G = GeometrySmith(_N, _V, _L, _roughness);
	float3 F = FresnelSchlick(HDotV, _F0);

	//Cook-Torrance Specular BRDF
	float3 numerator = D * G * F;
	float denominator = max(4.0 * NDotV * NDotL, 0.0001); //Prevent divide by zero
	float3 specular = numerator / denominator;

	
	//----DIFFUSE----//
	float3 kS = F; //specular power determined by fresnel
	float3 kD = 1.0 - kS; //gotta conserve that energy
	kD *= (1.0 - _metallic); //metals absorb all refracted light and convert to heat
	float3 diffuse = kD * _albedo / 3.141592653589; //todo: read this https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/


	float angleAttenuation = NDotL;
	return (diffuse + specular) * _radiance * NDotL;
}



float3 FresnelSchlickRoughness(float _cosTheta, float3 _F0, float _roughness)
{
	//For smooth surfaces, reflectivity at 90 degrees is 1.0 (pure white)
	//For rough surfaces, dampen the 1.0 based on roughness
	//I.e.: reflectivity increases as angle increases and decreases as roughness increases
    return _F0 + (max(1.0 - _roughness, _F0) - _F0) * pow(clamp(1.0 - _cosTheta, 0.0, 1.0), 5.0);
}



float3 CalculateIBL(float3 _albedo, float _roughness, float _metallic, float3 _F0, float3 _N, float3 _V)
{
	float NDotV = max(dot(_N, _V), 0.0);
	float3 R = reflect(-_V, _N);
	
	//Specular IBL (prefiltered environment map + brdf lut)
	const float MAX_REFLECTION_LOD = 4.0; //todo: use the actual number of mip levels
	float3 prefilteredColour = g_cubemaps[NonUniformResourceIndex(PC(prefilterCubemapIndex))].SampleLevel(g_samplers[NonUniformResourceIndex(PC(samplerIndex))], R, _roughness * MAX_REFLECTION_LOD).rgb;
	float2 brdfLUT = g_textures[NonUniformResourceIndex(PC(brdfLUTIndex))].Sample(g_samplers[NonUniformResourceIndex(PC(brdfLUTSamplerIndex))], float2(NDotV, _roughness)).rg;
	float3 specularIBL = prefilteredColour * (_F0 * brdfLUT.x + brdfLUT.y);

	//Energy conservation
	float3 kS = FresnelSchlickRoughness(max(dot(_N, _V), 0.0), _F0, _roughness);
	float3 kD = 1.0 - kS;
	kD *= (1.0 - _metallic);	

	//Diffuse IBL (irradiance)
	float3 irradiance = g_cubemaps[NonUniformResourceIndex(PC(irradianceCubemapIndex))].Sample(g_samplers[NonUniformResourceIndex(PC(samplerIndex))], _N).rgb;
	float3 diffuseIBL = irradiance * _albedo * kD;

	return diffuseIBL + specularIBL;
}



[shader("pixel")]
float4 FSMain(VertexOutput vertexOutput) : SV_TARGET
{
    NK::PBRMaterial material = g_materials[NonUniformResourceIndex(PC(materialBufferIndex))];
    SamplerState linearSampler = g_samplers[NonUniformResourceIndex(PC(samplerIndex))];
	StructuredBuffer<LightData> lightBuffer = g_lightData[NonUniformResourceIndex(PC(lightDataBufferIndex))];

    float4 albedoSample = g_textures[NonUniformResourceIndex(material.baseColourIdx)].Sample(linearSampler, vertexOutput.texCoord);
    float4 normalSample = g_textures[NonUniformResourceIndex(material.normalIdx)].Sample(linearSampler, vertexOutput.texCoord);
    float4 metallicSample = g_textures[NonUniformResourceIndex(material.metalnessIdx)].Sample(linearSampler, vertexOutput.texCoord);
    float4 roughnessSample = g_textures[NonUniformResourceIndex(material.roughnessIdx)].Sample(linearSampler, vertexOutput.texCoord);
    float4 emissiveSample = g_textures[NonUniformResourceIndex(material.emissiveIdx)].Sample(linearSampler, vertexOutput.texCoord);

    float3 albedo = pow(albedoSample.rgb, 2.2); //srgb -> linear
    float3 normal = normalize(mul(vertexOutput.TBN, normalSample.rgb * 2.0 - 1.0));
    float metallic = metallicSample.g;
    float roughness = roughnessSample.b;
    float3 emissive = pow(emissiveSample.rgb, 2.2); //srgb -> linear


    float3 V = normalize(vertexOutput.camPos - vertexOutput.fragPos); //frag pos to camera

	//Base reflectance
	float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);


	//Direct lighting
	float3 totalDirectLighting = float3(0.0f, 0.0f, 0.0f);
	for (uint i = 0; i < PC(numLights); ++i)
	{
		LightData light = lightBuffer[i];

		//Shadows - if point is in shadow from this light, just skip this light
		if (light.type == 2) //Point Light
		{
			if (PointInPointLightShadow(vertexOutput.worldPos, light.position, light.shadowMapIndex, linearSampler))
			{
				continue;
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
		}


		//Point is not in shadow from this light source - add up its contribution

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
				float3 radiance = light.colour * light.intensity;
				float3 directLighting = CalculateDirectLight(albedo, metallic, roughness, F0, normal, V, lightDirToFragPos, radiance);
				totalDirectLighting += directLighting * attenuation;
			}
		}
	}


	float3 ambientLighting = CalculateIBL(albedo, roughness, metallic, F0, normal, V);
	if (material.hasAO)
	{
		ambientLighting *= g_textures[NonUniformResourceIndex(material.aoIdx)].Sample(linearSampler, vertexOutput.texCoord).r;
	}


	//Skip ambient for now
    float3 colour = totalDirectLighting + ambientLighting + emissive;

    return float4(colour, 1.0);
}