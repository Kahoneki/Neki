#include <Types/ShaderMacros.hlsli>

struct VertexInput
{
	ATTRIBUTE(float2, pos, NK::SHADER_ATTRIBUTE_LOCATION_POSITION, POSITION);
	ATTRIBUTE(float3, colour, NK::SHADER_ATTRIBUTE_LOCATION_COLOUR_0, COLOR0);
};

struct VertexOutput
{
	float4 pos : SV_POSITION;
	float3 colour : COLOR0;
};

VertexOutput VSMain(VertexInput input)
{	
	VertexOutput output;
	output.pos = float4(input.pos, 0.0, 1.0);
	output.colour = input.colour;

	return output;
}