#include <Types/ShaderAttribute.hlsli>

struct VertexOutput
{
	ATTRIBUTE(float4, pos, NK::SHADER_ATTRIBUTE_LOCATION_POSITION, SV_POSITION);
	ATTRIBUTE(float3, colour, NK::SHADER_ATTRIBUTE_LOCATION_COLOUR_0, COLOR0);
};

VertexOutput VSMain(uint vertexID : SV_VertexID)
{
    float2 positions[3] =
    {
        float2(0.0f, 0.5f), //Top center
		float2(0.5f, -0.5f), //Bottom right
		float2(-0.5f, -0.5f) //Bottom left
    };
	
    float3 colours[3] =
    {
        float3(1.0, 0.0, 0.0),
		float3(0.0, 1.0, 0.0),
		float3(0.0, 0.0, 1.0)
    };
	
    VertexOutput output;
    output.pos = float4(positions[vertexID], 0.0, 1.0);
    output.colour = colours[vertexID];

    return output;
}