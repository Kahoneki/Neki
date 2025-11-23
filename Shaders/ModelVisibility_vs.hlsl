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

[[vk::binding(0,0)]] ConstantBuffer<CamData> g_camData[] : register(b0, space0);
[[vk::binding(0,0)]] StructuredBuffer<float4x4> g_modelMatrices[] : register(t0, space0);

PUSH_CONSTANTS_BLOCK(
	uint camDataBufferIndex;
	uint modelMatricesBufferIndex;
	uint modelVisibilityBufferIndex;
);


VertexOutput VSMain(VertexInput input, uint instanceID : SV_InstanceID)
{
    VertexOutput output;
	CamData camData = g_camData[NonUniformResourceIndex(PC(camDataBufferIndex))];
	float4x4 modelMat = g_modelMatrices[NonUniformResourceIndex(PC(modelMatricesBufferIndex))][instanceID];
	
	float4 worldPos = mul(modelMat, float4(input.pos, 1.0));
	output.pos = mul(camData.projMat, mul(camData.viewMat, worldPos));
	output.instanceID = instanceID;

    return output;
}