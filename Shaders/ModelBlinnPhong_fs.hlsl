#include <Types/ShaderMacros.hlsli>

struct VertexOutput
{
	float4 pos : SV_POSITION;
	float2 texCoord : TEXCOORD0;
	float3 fragPos : FRAG_POS;
	float3 worldNormal : WORLD_NORMAL;
	float3 camPos : CAM_POS;
	float3x3 TBN : TBN;	
};

float4 FSMain(VertexOutput vertexOutput) : SV_TARGET
{
	return float4(0.0, 1.0, 0.0, 1.0);
}