#include <Types/ShaderMacros.hlsli>

struct VertexOutput
{
	float4 pos : SV_POSITION;
	float2 texCoord : TEXCOORD;
};

[[vk::binding(0,0)]] Texture2D g_textures[] : register(t0, space0);
[[vk::binding(1,0)]] SamplerState g_samplers[] : register(s0, space0);

PUSH_CONSTANTS_BLOCK(
	uint sceneColourIndex;
	uint sceneDepthIndex;
	uint satTextureIndex;
	uint samplerIndex;

	float nearPlane;
	float farPlane;
	float focalDistance;
	float focalDepth;
	float maxBlurRadius;
	uint dofDebugMode;

	float acesExposure;
);



//Credit: https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
float3 ACESFilm(float3 _x)
{
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
	return saturate((_x * (a * _x + b)) / (_x * (c * _x + d) + e));
}



float LineariseDepth(float _depth, float _near, float _far)
{
	float z = _depth * 2.0 - 1.0; //Transform to NDC (-1 to 1)
    float linearDepth = (2.0 * _near * _far) / (_far + _near - z * (_far - _near));
	return linearDepth;
}



float3 GetBlurColour(int _radius, int2 _centre, int2 _maxBounds)
{
	if (_radius <= 0) { return 0; }

	int2 boxMin = max(_centre - _radius, 0);
	int2 boxMax = min(_centre + _radius, _maxBounds);

	int2 satMin = boxMin - 1; 
	int2 satMax = boxMax;

	//Sample SAT
	//Sat formula for box sum: D + A - B - C
	//(A = top left, B = top right, C = bottom left, D = bottom right)
	//Also, since SATs are precision-sensitive floats, using a sampler would ruin their values, so use Load() instead
	float4 D = g_textures[NonUniformResourceIndex(PC(satTextureIndex))].Load(int3(satMax, 0));
	float4 A = (satMin.x >= 0 && satMin.y >= 0) ? g_textures[NonUniformResourceIndex(PC(satTextureIndex))].Load(int3(satMin, 0)) : 0;
	float4 B = (satMin.y >= 0) ? g_textures[NonUniformResourceIndex(PC(satTextureIndex))].Load(int3(satMax.x, satMin.y, 0)) : 0;
	float4 C = (satMin.x >= 0) ? g_textures[NonUniformResourceIndex(PC(satTextureIndex))].Load(int3(satMin.x, satMax.y, 0)) : 0;

	float3 sum = (D + A - B - C).rgb;
	float area = (float)((boxMax.x - boxMin.x + 1) * (boxMax.y - boxMin.y + 1));
	return (area > 0.0) ? sum / area : 0; //just a safety check - dont divide by zero !!
}



float3 DepthOfField(float3 _colour, float _depth, int2 _pixelCoords)
{
	uint width, height;
    g_textures[NonUniformResourceIndex(PC(satTextureIndex))].GetDimensions(width, height);
    int2 maxBounds = int2(width - 1, height - 1);

	float linearDepth = LineariseDepth(_depth, PC(nearPlane), PC(farPlane));
	float blurRadius;
	
	//Avoid divide-by-zero
	if (_depth == 0)
	{
		blurRadius = 1.0f; //Default blur radius
	}
	else
	{
		//Calculate a "circle of confusion" (fancy term for the blur radius)
		float distanceFromFocalDistance = abs(linearDepth - PC(focalDistance)); //The closer the depth is to the focal distance, the lower the blur radius will be -> the less blury it will be
		float blurStrength01 = smoothstep(0.0f, PC(focalDepth), distanceFromFocalDistance);
		blurRadius = lerp(1.0f, PC(maxBlurRadius), blurStrength01);
	}

	if (PC(dofDebugMode))
	{
		float blurAmount01 = blurRadius / PC(maxBlurRadius);
		return float4(blurAmount01, blurAmount01, blurAmount01, 1.0f);
	}

	//If blur is minimal, just return the original sharp colour
	if (blurRadius <= 1.0f)
	{
		return _colour;
	}


	//Take 2 samples for less blocky looking result
	int rLow = (int)floor(blurRadius);
	int rHigh = rLow + 1;
	float t = frac(blurRadius);
	float3 cLow  = (rLow < 1) ? _colour : GetBlurColour(rLow, _pixelCoords, maxBounds);
	float3 cHigh = GetBlurColour(rHigh, _pixelCoords, maxBounds);
	return lerp(cLow, cHigh, t);
}



[shader("pixel")]
float4 FSMain(VertexOutput vertexOutput) : SV_TARGET
{
	SamplerState linearSampler = g_samplers[NonUniformResourceIndex(PC(samplerIndex))];
	float3 sceneColour = g_textures[NonUniformResourceIndex(PC(sceneColourIndex))].Sample(linearSampler, vertexOutput.texCoord).rgb;
	float sceneDepth = g_textures[NonUniformResourceIndex(PC(sceneDepthIndex))].Sample(linearSampler, vertexOutput.texCoord).r;

	sceneColour = DepthOfField(sceneColour, sceneDepth, vertexOutput.pos);
	sceneColour *= PC(acesExposure);
	sceneColour = ACESFilm(sceneColour);

	float d = LineariseDepth(sceneDepth, PC(nearPlane), PC(farPlane));

	return float4(sceneColour, 1.0);
}