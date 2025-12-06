#include <Types/ShaderMacros.hlsli>

#pragma enable_dxc_extensions

struct VertexOutput
{
	float4 pos : SV_POSITION;
	nointerpolation uint instanceID : INSTANCE_ID;
};

[[vk::binding(0,0)]] RWStructuredBuffer<uint> g_modelVisibility[] : register(u0, space0);

PUSH_CONSTANTS_BLOCK(
	uint camDataBufferIndex;
	uint modelMatricesBufferIndex;
	uint modelVisibilityBufferIndex;
);


//By using the earlydepthstencil attribute, the GPU is forced to perform early depth culling
//=> This means that this fragment shader is only ran if the depth test passed (if there's at least one fragment visible)
//=> This means the model is visible
[earlydepthstencil]
void FSMain(VertexOutput vertexOutput)
{
	//This fragment shader is ran for each visible fragment, usually this would result in a race condition when trying to write to the buffer like this
	//However, in this instance it's okay since every thread is just writing the same constant value
	g_modelVisibility[NonUniformResourceIndex(PC(modelVisibilityBufferIndex))][vertexOutput.instanceID] = 1u;

	//printf("hey");
}