RWTexture2D<uint> input_data : register(u0);
RWTexture2D<uint> summed_area_table : register(u1);

struct DataProperties
{
    uint max_value;
};

ConstantBuffer<DataProperties> data_properties : register(b1, space0);

[numthreads(1, 64, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint width;
    uint height;
    summed_area_table.GetDimensions(width, height);
    
    if (dispatchThreadID.y > height)
    {
        return;
    }
    
    uint current_sum = 0;
    uint2 index = uint2(0, dispatchThreadID.y);
    
    for (uint i = 0; i < width; i++)
    {
        index.x = i;
        current_sum += input_data[index];
        summed_area_table[index] = min(current_sum, data_properties.max_value);
    }
}