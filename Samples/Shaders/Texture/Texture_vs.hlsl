#include <Types/ShaderMacros.hlsli>

struct VertexInput
{
	ATTRIBUTE(float2, pos, NK::SHADER_ATTRIBUTE_LOCATION_POSITION, POSITION);
};

struct VertexOutput
{
	float4 pos : SV_POSITION;
};

VertexOutput VSMain(VertexInput input)
{	
    VertexOutput output;
    output.pos = float4(input.pos, 0.0, 1.0);

    return output;
}