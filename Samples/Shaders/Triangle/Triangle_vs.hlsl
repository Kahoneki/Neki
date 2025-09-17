float4 VSMain(uint vertexID : SV_VertexID) : SV_POSITION
{
	float2 positions[3] = {
		float2(0.0f, -0.5f), //Bottom center
		float2(0.5f,  0.5f), //Top right
		float2(-0.5f, 0.5f)  //Top left
	};

	return float4(positions[vertexID], 0.0f, 1.0f);
}