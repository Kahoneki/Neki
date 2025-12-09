#include <Types/ShaderMacros.hlsli>


struct VertexInput
{
	ATTRIBUTE(float3, pos, NK::SHADER_ATTRIBUTE_LOCATION_POSITION, POSITION);
};

struct VertexOutput
{
	float4 pos : SV_POSITION; //Output in clip space
	float3 texCoord : TEXCOORD0; //Direction used for sampling skybox cubemap
};


struct CamData
{
	float4x4 viewMat;
	float4x4 projMat;
	float4 pos;
};

[[vk::binding(0,0)]] ConstantBuffer<CamData> g_camData[] : register(b0, space0);


PUSH_CONSTANTS_BLOCK(
	float4x4 modelMat;
	uint camDataBufferIndex;
	uint numLights;
	uint lightDataBufferIndex;
	uint shadowMapIndex;
	uint skyboxCubemapIndex;
	uint materialBufferIndex;
	uint samplerIndex;
);


VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
	CamData camData = g_camData[NonUniformResourceIndex(PC(camDataBufferIndex))];

	float4x4 viewNoTrans = camData.viewMat;
	viewNoTrans[0][3] = viewNoTrans[1][3] = viewNoTrans[2][3] = 0.0;

	output.pos = mul(camData.projMat, mul(viewNoTrans, float4(input.pos, 1.0f)));
	output.pos = output.pos.xyww; //After perspective division, depth will be 1.0f
	
	//Use the local-space position as a 3d direction for the texcoord used to sample the cubemap
	output.texCoord = input.pos;

    return output;
}