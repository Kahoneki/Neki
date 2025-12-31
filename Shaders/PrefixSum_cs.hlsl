#include <Types/ShaderMacros.hlsli>

#pragma enable_dxc_extensions

#define WORKGROUP_SIZE 1024


[[vk::binding(0,0)]] Texture2D<float4> g_textures[] : register(t0, space0);
[[vk::binding(0,0)]] RWTexture2D<float4> g_rwTextures[] : register(u0, space0);


PUSH_CONSTANTS_BLOCK(
    uint inputIdx;
    uint outputIdx;
    uint2 dimensions; //x = width, y = height
);


//Shared memory for the parallel prefix sum - each thread calculates 2 elements
groupshared float4 sharedData[WORKGROUP_SIZE * 2];


[numthreads(WORKGROUP_SIZE, 1, 1)]
void CSMain(uint3 GroupThreadID : SV_GroupThreadID, uint3 GroupID : SV_GroupID)
{
    Texture2D<float4> inputTex = g_textures[NonUniformResourceIndex(PC(inputIdx))];
    RWTexture2D<float4> outputTex = g_rwTextures[NonUniformResourceIndex(PC(outputIdx))];
    
	//Prevent INF
	static const float MAX_VAL = 64000.0f;
    
    uint id = GroupThreadID.x;
    uint rowIndex = GroupID.x; //one group per row
    
    float4 runningSum = float4(0,0,0,0);
    
    //Loop through the row in chunks of 2048 (WORKGROUP_SIZE * 2)
    for (uint segmentOffset = 0; segmentOffset < PC(dimensions).x; segmentOffset += WORKGROUP_SIZE * 2)
    {
        int2 p1 = int2(segmentOffset + id * 2, rowIndex);
        int2 p2 = int2(segmentOffset + id * 2 + 1, rowIndex);
        
        float4 val1 = (p1.x < PC(dimensions).x && p1.y < PC(dimensions).y) ? inputTex[p1] : float4(0,0,0,0);
        float4 val2 = (p2.x < PC(dimensions).x && p2.y < PC(dimensions).y) ? inputTex[p2] : float4(0,0,0,0);
        
        sharedData[id * 2] = min(val1, MAX_VAL);
        sharedData[id * 2 + 1] = min(val2, MAX_VAL);
        
        //Synchronise threads to make sure they've all sent their respective input data into the sharedData array
        GroupMemoryBarrierWithGroupSync();
    
        //blelloch/hillis-steele scan
        const uint steps = (uint)log2(WORKGROUP_SIZE) + 1;
        uint readIndex;
        uint writeIndex;
        uint mask;

        [unroll]
        for (uint step = 0; step < steps; step++)
        {
            //Calculate read and write index in the shared array
            mask = (1 << step) - 1;
            readIndex = ((id >> step) << (step + 1)) + mask; //On first pass, 1 thread per read index, on second pass, 2 threads per read index, then 4, then 8, etc.
            writeIndex = readIndex + 1 + (id & mask); //Always 1 thread per write index

            //Accumulate the read data of all threads into write index
            if (writeIndex < (WORKGROUP_SIZE * 2))
            {
                sharedData[writeIndex] += sharedData[readIndex];
            }

            //Synchronise to make sure all threads are caught up with our write
            GroupMemoryBarrierWithGroupSync();
        }

        //Add the running sum from all previous segments
        sharedData[id * 2] += runningSum;
        sharedData[id * 2 + 1] += runningSum;
        
        //Write shared data to output image, transposing data to prepare for next dispatch on columns. After two passes, image will be back to original orientation
        if (p1.x < PC(dimensions).x && p1.y < PC(dimensions).y)
        {
            outputTex[int2(p1.y, p1.x)] = sharedData[id * 2];
        }
        if ((p2.x) < PC(dimensions).x && p2.y < PC(dimensions).y)
        {
            outputTex[int2(p2.y, p2.x)] = sharedData[id * 2 + 1];
        }
        
        //Update running sum for the next segment
        runningSum = sharedData[WORKGROUP_SIZE * 2 - 1];
        GroupMemoryBarrierWithGroupSync();
    }
}