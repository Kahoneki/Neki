//VERY temp


#include <Types/ShaderMacros.hlsli>
#include <Types/Materials.h>

#pragma enable_dxc_extensions


struct VertexOutput
{
	float4 pos : SV_POSITION;
	float3 worldPos : WORLD_POS;
	float2 texCoord : TEXCOORD0;
	float3 fragPos : FRAG_POS;
	float3 worldNormal : WORLD_NORMAL;
	float3 camPos : CAM_POS;
	float3x3 TBN : TBN;
};



[[vk::binding(0,0)]] Texture2D g_textures[] : register(t0, space1);
[[vk::binding(0,0)]] TextureCube g_cubemaps[] : register(t0, space2);
[[vk::binding(1,0)]] SamplerState g_samplers[] : register(s0, space0);
[[vk::binding(0,0)]] ConstantBuffer<NK::BlinnPhongMaterial> g_materials[] : register(b0, space0);
[[vk::binding(0,0)]] ByteAddressBuffer g0Buffer[] : register(t0, space0);
[[vk::binding(0,0)]] ByteAddressBuffer g1Buffer[] : register(t0, space0);
[[vk::binding(0,0)]] ByteAddressBuffer mlpBuffer[] : register(t0, space0);



PUSH_CONSTANTS_BLOCK(
	float4x4 modelMat;
	uint camDataBufferIndex;

	uint numLights;
	uint lightDataBufferIndex;

	uint skyboxCubemapIndex;
	uint irradianceCubemapIndex;
	uint prefilterCubemapIndex;
	uint brdfLUTIndex;
	//uint brdfLUTSamplerIndex;
	uint mlpBufferIndex;
	
	//VERY temp
	uint g0BufferIndex;
	uint g1BufferIndex;
	
	float maxIrradiance;
);



float HardGELU(float x)
{
	//yikes
	if (x < -1.5f) { return 0; }
	else if (x > 1.5f) { return x; }
	else { return x/3 * (x + 1.5f); }
}



struct G0Features
{
	float channels[12];
};



struct G1Features
{
	float channels[10];
};



G0Features SampleG0(int2 p)
{
	G0Features output;
	for (uint c=0; c<12; ++c)
	{
		//4 values are packed into every byte in the buffer
		uint linearIdx = uint(c * 2048 * 2048 + p.y * 2048 + p.x); //Index of the packed value
		uint byteOffset = linearIdx / 4; //Index of the actual byte to be loaded that contains the desired value
		uint packed = (g0Buffer[PC(g0BufferIndex)].Load(byteOffset & ~3) >> ((byteOffset & 3) * 8)) & 0xFF;
		
		//linearIdx % 4 gives which of the 4 positions within the byte the value is stored (0,1,2,3)
		//(3 - (linearIdx % 4)) * 2 computes the bit shift to reach that position
		//linearIdx % 4		shift		extracts
		//0					6			bits 7-6 (val0)
		//1					4			bits 5-4 (val1)
		//2					2			bits 3-2 (val2)
		//3					0			bits 1-0 (val3)
		uint shift = (3 - (linearIdx % 4)) * 2;
		
		//Shift desired two bits to the bottom and mask with 0b11 to isolate
		uint raw = (packed >> shift) & 0b11;
		
		//Re-quantise from {0b00 (0), 0b01 (1), 0b10 (2), 0b11 (3)} to {0, 1/3, 2/3, 1}
		float value = raw / 3.0;
		
		output.channels[c] = value;
	}
	
	return output;
}



G1Features SampleG1(int2 p)
{
	G1Features output;
	for (uint c=0; c<10; ++c)
	{
		//2 values are packed into every byte in the buffer
		uint linearIdx = uint(c * 1024 * 1024 + p.y * 1024 + p.x); //Index of the packed value
		uint byteOffset = linearIdx / 2; //Index of the actual byte to be loaded that contains the desired value
		uint packed = (g1Buffer[PC(g1BufferIndex)].Load(byteOffset & ~3) >> ((byteOffset & 3) * 8)) & 0xFF;
		uint raw = (linearIdx % 2 == 0) ? (packed >> 4) : (packed & 0xF);
		
		//Re-quantise
		float value = raw / 15.0;
		
		output.channels[c] = value;
	}
	
	return output;
}



float3 EvaluateMLP(float2 uv)
{
	float2 p = uv * 4096.0f;
	float2 local = fmod(p, 256.0f) / 256.0f;
	local = local * 2.0f - 1.0f;
	
	float encodings[(8 * 2 * 2) + (1) + (12 * 4) + (10)]; //Size=91
	for (uint i=0; i<8; ++i)
	{
		float scale = exp2(float(i)) * 3.141592653589;
		encodings[i*4] = sin(scale * local.x);
		encodings[i*4+1] = sin(scale * local.y);
		encodings[i*4+2] = cos(scale * local.x);
		encodings[i*4+3] = cos(scale * local.y);
	}
	encodings[32] = 0.0f; //normalised lod
	
	//G0
	int w = 2048;
	int h = 2048;
	p = int2(int(uv.x * (w-1)), int(uv.y * (h-1)));
	int x0 = clamp(int(floor(p.x)), 0, w-1);
	int x1 = clamp(x0+1, 0, w-1);
	int y0 = clamp(int(floor(p.y)), 0, h-1);
	int y1 = clamp(y0+1, 0, h-1);
	{
		G0Features s00 = SampleG0(int2(x0, y0));
		for (uint c=0; c<12; ++c) { encodings[33+c] = s00.channels[c]; }
		G0Features s01 = SampleG0(int2(x0, y1));
		for (uint c=0; c<12; ++c) { encodings[33+12+c] = s01.channels[c]; }
		G0Features s10 = SampleG0(int2(x1, y0));
		for (uint c=0; c<12; ++c) { encodings[33+24+c] = s10.channels[c]; }
		G0Features s11 = SampleG0(int2(x1, y1));
		for (uint c=0; c<12; ++c) { encodings[33+36+c] = s11.channels[c]; }
	}
	
	//G1
	w = 1024;
	h = 1024;
	float fx = uv.x * (w - 1);
	float fy = uv.y * (h - 1);
	x0 = clamp(int(floor(fx)), 0, w-1);
	x1 = clamp(x0+1, 0, w-1);
	y0 = clamp(int(floor(fy)), 0, h-1);
	y1 = clamp(y0+1, 0, h-1);
	float2 weight = float2(fx-x0, fy-y0);
	{
		G1Features s00 = SampleG1(int2(x0, y0));
		G1Features s01 = SampleG1(int2(x0, y1));
		G1Features s10 = SampleG1(int2(x1, y0));
		G1Features s11 = SampleG1(int2(x1, y1));
		for (uint c=0; c<10; ++c)
		{
			float latent = (1-weight.x) * (1-weight.y) * s00.channels[c];
			latent += (1-weight.x) * weight.y * s01.channels[c];
			latent += weight.x * (1-weight.y) * s10.channels[c];
			latent += weight.x * weight.y * s11.channels[c];
			encodings[81+c] = latent;
		}
	}
	
	
	//MLP
	uint weightOffset = 12; //Skip numLayers + in_features + out_features (3 * uint32)
	uint biasOffset = weightOffset + 64 * 91 * 2; //Weight data size in bytes (*2 because fp16)
	float hidden1[64];
	for (uint o = 0; o < 64; ++o)
	{
		float sum = 0;
		for (uint i=0; i<91; ++i)
		{
			uint byteOffset = weightOffset + (o*91+i)*2;
			uint packed = mlpBuffer[PC(mlpBufferIndex)].Load(byteOffset & ~3);
			float w_val = f16tof32(packed >> ((byteOffset & 2) * 8));
			sum += w_val * encodings[i];
		}
		uint bOffset = biasOffset + o * 2;
		uint bPacked = mlpBuffer[PC(mlpBufferIndex)].Load(bOffset & ~3);
		float b_val = f16tof32(bPacked >> ((bOffset & 2) * 8));
		sum += b_val;
		
		//Activation
		hidden1[o] = HardGELU(sum);
	}
	
	weightOffset = biasOffset + 64*2 + 8;
	biasOffset = weightOffset + 64*64*2;
	float hidden2[64];
	for (uint o = 0; o < 64; ++o)
	{
		float sum = 0;
		for (uint i=0; i<64; ++i)
		{
			uint byteOffset = weightOffset + (o*64+i)*2;
			uint packed = mlpBuffer[PC(mlpBufferIndex)].Load(byteOffset & ~3);
			float w_val = f16tof32(packed >> ((byteOffset & 2) * 8));
			sum += w_val * hidden1[i];
		}
		uint bOffset = biasOffset + o * 2;
		uint bPacked = mlpBuffer[PC(mlpBufferIndex)].Load(bOffset & ~3);
		float b_val = f16tof32(bPacked >> ((bOffset & 2) * 8));
		sum += b_val;
		
		//Activation
		hidden2[o] = HardGELU(sum);
	}
	
	weightOffset = biasOffset + 64*2 + 8;
	biasOffset = weightOffset + 64*9*2;
	float output[9];
	for (uint o = 0; o < 9; ++o)
	{
		float sum = 0;
		for (uint i=0; i<64; ++i)
		{
			uint byteOffset = weightOffset + (o*64+i)*2;
			uint packed = mlpBuffer[PC(mlpBufferIndex)].Load(byteOffset & ~3);
			float w_val = f16tof32(packed >> ((byteOffset & 2) * 8));
			sum += w_val * hidden2[i];
		}
		uint bOffset = biasOffset + o * 2;
		uint bPacked = mlpBuffer[PC(mlpBufferIndex)].Load(bOffset & ~3);
		float b_val = f16tof32(bPacked >> ((bOffset & 2) * 8));
		sum += b_val;
		
		output[o] = sum;
	}
	
	return float3(output[0], output[1], output[2]);
}



[shader("pixel")]
float4 FSMain(VertexOutput vertexOutput) : SV_TARGET
{
	float3 networkOutputRGB = EvaluateMLP(vertexOutput.texCoord);
	
	networkOutputRGB = clamp(networkOutputRGB, 0.0f, 1.0f);
	float3 linearRGB = pow(networkOutputRGB, 2.2f);

	return float4(linearRGB, 1.0f);
}