float4 VSMain(uint vertexID : SV_VertexID) : SV_POSITION
{
	float2 positions[3] = {
		float2(0.0f, 0.5f),		//Top center
		float2(0.5f, -0.5f),	//Bottom right
		float2(-0.5f, -0.5f)	//Bottom left
	};

	return float4(positions[vertexID], 0.0f, 1.0f);
}