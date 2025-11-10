#include <Types/ShaderMacros.hlsli>


struct VertexInput
{
	ATTRIBUTE(float3, pos, NK::SHADER_ATTRIBUTE_LOCATION_POSITION, POSITION);
	ATTRIBUTE(float2, texCoord, NK::SHADER_ATTRIBUTE_LOCATION_TEXCOORD_0, TEXCOORD0);
};

struct VertexOutput
{
	float4 pos : SV_POSITION;
	float2 texCoord : TEXCOORD;
};


VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
	
    output.pos = float4(input.pos, 1.0);
	output.texCoord = input.texCoord;

    return output;
}