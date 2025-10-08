#include <Types/ShaderMacros.hlsli>


struct VertexInput
{
	ATTRIBUTE(float3, pos, NK::SHADER_ATTRIBUTE_LOCATION_POSITION, POSITION);
	ATTRIBUTE(float3, colour, NK::SHADER_ATTRIBUTE_LOCATION_COLOUR_0, COLOR0);
};

struct VertexOutput
{
	float4 pos : SV_POSITION;
	float3 colour : COLOR0;
};


struct CamData
{
	float4x4 viewMat;
	float4x4 projMat;
};

[[vk::binding(0,0)]] ConstantBuffer<CamData> g_camData[] : register(b0, space0);


PUSH_CONSTANTS_BLOCK(
	float4x4 modelMat;
	uint camDataIndex;
);


VertexOutput VSMain(VertexInput input)
{	
    VertexOutput output;
	CamData camData = g_camData[NonUniformResourceIndex(PC(camDataIndex))];

	float4x4 mvp = mul(camData.projMat, mul(camData.viewMat, PC(modelMat)));
	output.pos = mul(mvp, float4(input.pos, 1.0));
	
	//Pass through unchanged
	output.colour = input.colour;

    return output;
}