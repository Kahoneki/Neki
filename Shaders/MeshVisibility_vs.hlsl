#include <Types/ShaderMacros.hlsli>


struct VertexInput
{
	ATTRIBUTE(float3, pos, NK::SHADER_ATTRIBUTE_LOCATION_POSITION, POSITION);
};

struct VertexOutput
{
	float4 pos : SV_POSITION;
	nointerpolation uint instanceID : INSTANCE_ID;
};

struct CamData
{
	float4x4 viewMat;
	float4x4 projMat;
	float4 pos;
};

struct ModelMatrixShaderData
{
	float4x4 modelMat;
	uint visibilityIndex;
	uint3 padding;
};

[[vk::binding(0,0)]] ConstantBuffer<CamData> g_camData[] : register(b0, space0);
[[vk::binding(0,0)]] StructuredBuffer<ModelMatrixShaderData> g_modelMatrices[] : register(t0, space0);

PUSH_CONSTANTS_BLOCK(
	uint camDataBufferIndex;
	uint modelMatricesBufferIndex;
	uint modelVisibilityBufferIndex;
);


VertexOutput VSMain(VertexInput input, uint instanceID : SV_InstanceID)
{
    VertexOutput output;
	CamData camData = g_camData[NonUniformResourceIndex(PC(camDataBufferIndex))];
	ModelMatrixShaderData data = g_modelMatrices[NonUniformResourceIndex(PC(modelMatricesBufferIndex))][instanceID];
	
	float4 worldPos = mul(data.modelMat, float4(input.pos, 1.0));
	output.pos = mul(camData.projMat, mul(camData.viewMat, worldPos));
	output.instanceID = data.visibilityIndex;

    return output;
}