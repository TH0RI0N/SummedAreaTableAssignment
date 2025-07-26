RWTexture2D<uint> summed_area_table : register(u1);

[numthreads(32, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint width;
    uint height;
    summed_area_table.GetDimensions(width, height);
    
    if (dispatchThreadID.x > width)
    {
        return;
    }
    
    uint current_sum = 0;
    uint2 index = uint2(dispatchThreadID.x, 0);
    
    for (uint i = 0; i < height; i++)
    {
        index.y = i;
        current_sum += summed_area_table[index];
        summed_area_table[index] = min(current_sum, 255);
    }
}