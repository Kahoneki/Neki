#include <Types/ShaderMacros.hlsli>

#pragma enable_dxc_extensions

struct VertexInput
{
	ATTRIBUTE(float3, pos, NK::SHADER_ATTRIBUTE_LOCATION_POSITION, POSITION);
	ATTRIBUTE(float3, normal, NK::SHADER_ATTRIBUTE_LOCATION_NORMAL, NORMAL);
	ATTRIBUTE(float2, texCoord, NK::SHADER_ATTRIBUTE_LOCATION_TEXCOORD_0, TEXCOORD0);
	ATTRIBUTE(float4, tangent, NK::SHADER_ATTRIBUTE_LOCATION_TANGENT, TANGENT);
};

struct VertexOutput
{
	float4 pos : SV_POSITION;
	float3 worldPos : WORLD_POS;
	float2 texCoord : TEXCOORD0;
	float3 fragPos : FRAG_POS;
	float3 worldNormal : WORLD_NORMAL;
	float3 camPos : CAM_POS;
	float3x3 TBN : TBN;	
	float3 bitangent : BITANGENT;
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

	uint skyboxCubemapIndex;
	uint irradianceCubemapIndex;
	uint prefilterCubemapIndex;
	uint brdfLUTIndex;
	uint brdfLUTSamplerIndex;

	uint materialBufferIndex;
	uint samplerIndex;
	
	float time;
	float waveAmplitude;
);


VertexOutput VSMain(VertexInput input, uint vertexID : SV_VertexID)
{
    VertexOutput output;
	CamData camData = g_camData[NonUniformResourceIndex(PC(camDataBufferIndex))];

    output.camPos = camData.pos.xyz;
	output.texCoord = input.texCoord;
	
	//Constants
	float freq = 3.0f;
	float speed = 5.0f;
	float amp = PC(waveAmplitude);
	
	//Displacement (non-linear horizontal shear based on height, in the shape of a sine wave)
	float angle =  speed * PC(time) + freq * input.pos.y;
	float offset = sin(angle) * amp;
	float3 manipulatedPos = input.pos;
	manipulatedPos.x += offset;
	
	//Derivative
	float derivative = cos(angle) * freq * amp;
	
	//Need to transform to the normal and tangent to correct the lighting
	//For x += y*k, the normal transforms by the inverse-transpose matrix:
	//[ 1  0  0 ]
	//[-k  1  0 ]
	//[ 0  0  1 ]
	float3 N = input.normal;
	float3 N_new;
	N_new.x = N.x;
	N_new.y = N.y - (derivative * N.x);
	N_new.z = N.z;
	N_new = normalize(N_new);
	
	//Tangent transforms by the regular shear matrix:
	//[ 1  k  0 ]
	//[ 0  1  0 ]
	//[ 0  0  1 ]
	float3 T = input.tangent.xyz;
	float3 T_new;
	T_new.x = T.x + (derivative * T.y);
	T_new.y = T.y;
	T_new.z = T.z;
	T_new = normalize(T_new);
	
	//Standard transforms (with manipulatedPos)
	float4 worldPos = mul(PC(modelMat), float4(manipulatedPos, 1.0));
	output.worldPos = worldPos.xyz;
	output.fragPos = float3(worldPos.xyz);
	output.pos = mul(camData.projMat, mul(camData.viewMat, worldPos));

	
	//TBN
    float sign = input.tangent.w;
	float3 T_os = float3(input.tangent.x, input.tangent.yz);
    float3 N_os = N_new;
    float3 B_os = cross(T_os, N_os) * sign;
    
	float3 T_world = normalize( mul(PC(modelMat), float4(T_os, 0.0)).xyz );
	float3 B_world = normalize( mul(PC(modelMat), float4(B_os, 0.0)).xyz );
	float3 N_world = normalize( mul(PC(modelMat), float4(N_os, 0.0)).xyz );

	output.TBN = transpose(float3x3(T_world, B_world, N_world));
	output.bitangent = B_world;
	output.worldNormal = N_world;

	
    return output;
}