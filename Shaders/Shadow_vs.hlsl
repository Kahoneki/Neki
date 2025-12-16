#include <Types/ShaderMacros.hlsli>


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
};


PUSH_CONSTANTS_BLOCK(
	float4x4 viewProjMat;
	float4x4 modelMat;
	float time;
	float waveAmplitude;
);


VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;

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
	output.pos = mul(PC(viewProjMat), worldPos);
	
    return output;
}