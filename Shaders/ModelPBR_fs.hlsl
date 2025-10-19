#include <Types/ShaderMacros.hlsli>
#include <Types/Materials.h>

#pragma enable_dxc_extensions

struct VertexOutput
{
	float4 pos : SV_POSITION;
	float2 texCoord : TEXCOORD0;
	float3 fragPos : FRAG_POS;
	float3 worldNormal : WORLD_NORMAL;
	float3 camPos : CAM_POS;
	float3x3 TBN : TBN;
	float3 bitangent : BITANGENT;
};

[[vk::binding(0,0)]] Texture2D g_textures[] : register(t0, space0);
[[vk::binding(0,1)]] TextureCube g_skyboxes[] : register(t0, space1);
[[vk::binding(1,0)]] SamplerState g_samplers[] : register(s0, space0);
[[vk::binding(0,0)]] ConstantBuffer<NK::PBRMaterial> g_materials[] : register(b0, space0);

PUSH_CONSTANTS_BLOCK(
	float4x4 modelMat;
	uint camDataBufferIndex;
	uint skyboxCubemapIndex;
	uint materialBufferIndex;
	uint samplerIndex;
);



float DistributionGGX(float3 _normal, float3 _halfway, float _roughness)
{
    float a = _roughness*_roughness;
    float a2 = a*a;
    float cosWeighting = max(dot(_normal, _halfway), 0.0);
    float cosWeighting2 = cosWeighting * cosWeighting;

    float numerator = a2;
    float denominator = (cosWeighting2 * (a2 - 1.0) + 1.0);
    denominator = max(3.141592653589 * denominator * denominator, 0.001);

    return numerator / denominator;
}



float GeometrySchlickGGX(float _cosWeighting, float _roughness)
{
    float r = _roughness + 1.0;
    float k = (r*r) / 8.0;

    float numerator = _cosWeighting;
    float denominator = _cosWeighting * (1.0 - k) + k;

    return numerator / denominator;
}



float GeometrySmith(float3 _normal, float3 _viewDirection, float3 _incomingLightDirection, float _roughness)
{
    float viewCosWeighting = max(dot(_normal, _viewDirection), 0.0);
    float lightCosWeighting = max(dot(_normal, -_incomingLightDirection), 0.0);
    float ggx2 = GeometrySchlickGGX(viewCosWeighting, _roughness);
    float ggx1 = GeometrySchlickGGX(lightCosWeighting, _roughness);
    return ggx1 * ggx2;
}



float3 FresnelSchlick(float _cosTheta, float3 _F0)
{
    return _F0 + (1.0 - _F0) * pow(clamp(1.0 - _cosTheta, 0.0, 1.0), 5.0);
}


[shader("pixel")]
float4 FSMain(VertexOutput vertexOutput) : SV_TARGET
{
	NK::PBRMaterial material = g_materials[NonUniformResourceIndex(PC(materialBufferIndex))];
	SamplerState linearSampler = g_samplers[NonUniformResourceIndex(PC(samplerIndex))];

    float4 albedoSample = g_textures[NonUniformResourceIndex(material.baseColourIdx)].Sample(linearSampler, vertexOutput.texCoord);
    float4 normalSample = g_textures[NonUniformResourceIndex(material.normalIdx)].Sample(linearSampler, vertexOutput.texCoord);
    float4 metallicSample = g_textures[NonUniformResourceIndex(material.metalnessIdx)].Sample(linearSampler, vertexOutput.texCoord);
    float4 roughnessSample = g_textures[NonUniformResourceIndex(material.roughnessIdx)].Sample(linearSampler, vertexOutput.texCoord);
	float4 aoSample = g_textures[NonUniformResourceIndex(material.aoIdx)].Sample(linearSampler, vertexOutput.texCoord);
    float4 emissiveSample = g_textures[NonUniformResourceIndex(material.emissiveIdx)].Sample(linearSampler, vertexOutput.texCoord);

    float3 albedo = albedoSample.rgb;
    float3 normal = normalize(mul(vertexOutput.TBN, normalSample.rgb * 2.0 - 1.0));
    float metallic = metallicSample.g;
    float roughness = roughnessSample.b;
    float ao = aoSample.r;
    float3 emissive = emissiveSample.rgb;

    //Base reflectance
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, albedo, metallic);


    //Calculate incoming radiance
    float3 lightPos = float3(2,-2,2);
    float3 radiantFlux = float3(23.47, 21.31, 20.79);
    float distance2 = dot(vertexOutput.fragPos - lightPos, vertexOutput.fragPos - lightPos);
    float attenuation = 1.0 / distance2;
    float3 incomingRadiance = radiantFlux * attenuation;


    //Calculate outgoing radiance
    float3 incomingDirection = normalize(vertexOutput.fragPos - lightPos);
    float3 viewDir = normalize(vertexOutput.camPos - vertexOutput.fragPos);
    float3 halfwayDir = normalize(-incomingDirection + viewDir);

    //Cook-Torrance BRDF
    float NDF = DistributionGGX(normal, halfwayDir, roughness);
    float G = GeometrySmith(normal, viewDir, incomingDirection, roughness);
    float3 F = FresnelSchlick(max(dot(halfwayDir, viewDir), 0.0), F0);

    float3 specularStrength = F;
    float3 diffuseStrength = float3(1.0, 1.0, 1.0) - specularStrength;
    diffuseStrength *= 1.0 - metallic; //Conductors (metals) don't have any diffuse reflectance

    float3 numerator = NDF * G * F;
    float denominator = max(4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, -incomingDirection), 0.0), 0.001);
    float3 specular = numerator / denominator;

    float cosWeighting = max(dot(normal, -incomingDirection), 0.0);
    float3 outgoingRadiance = (diffuseStrength * albedo / 3.141592653589 + specular) * incomingRadiance * cosWeighting;


    //Ambient
	float ambientStrength = 0.5f;
	float3 reflectionDir = reflect(-viewDir, normal);
    float4 skyboxSample = g_skyboxes[NonUniformResourceIndex(PC(skyboxCubemapIndex))].Sample(linearSampler, reflectionDir);
	float3 ambient = skyboxSample.rgb * albedo * ao * ambientStrength;


    float3 colour = ambient + emissive + outgoingRadiance;
	return float4(colour, 1.0);
}