#include "SummedAreaTableGeneratorGpuImpl.h"

#include "constants.h"
#include "DirectXHelper.h"
#include "d3dx12.h"

#include <chrono>
#include <iostream>
#include <thread>

static const float THREAD_GROUP_SIZE = 64.0f; // Numthreads in the compute shaders needs to be changed also

SummedAreaTableGeneratorGpuImpl::SummedAreaTableGeneratorGpuImpl()
{
    setup_shaders();
  
    switch (DATA_NUM_OF_BITS)
    {
        case 8:
            mDataFormat = DXGI_FORMAT_R8_UINT;
            break;
        case 16:
            mDataFormat = DXGI_FORMAT_R16_UINT;
            break;
        case 32:
            mDataFormat = DXGI_FORMAT_R32_UINT;
            break;
    }
}

float SummedAreaTableGeneratorGpuImpl::generate(const DataContainer& data_in, DataContainer& data_out)
{
    create_input_texture(data_in);
    create_output_texture(data_in);

    auto start = std::chrono::high_resolution_clock::now();
    compute_summed_area_table(data_in);
    auto end = std::chrono::high_resolution_clock::now();

    readback_output_data(data_in, data_out);

    return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0f;
}

void SummedAreaTableGeneratorGpuImpl::create_input_texture(const DataContainer& input_data)
{
    // Create the compute shader input texture
    D3D12_RESOURCE_DESC texture_description{};
    texture_description.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    texture_description.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texture_description.Width = input_data.width;
    texture_description.Height = input_data.height;
    texture_description.MipLevels = 1;
    texture_description.DepthOrArraySize = 1;
    texture_description.SampleDesc.Count = 1;
    texture_description.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texture_description.Format = mDataFormat;
    D3D12_HEAP_PROPERTIES default_heap = D3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    DirectXHelper::check_result(DirectXHelper::instance()->get_device()->CreateCommittedResource(&default_heap, D3D12_HEAP_FLAG_NONE,
        &texture_description, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&mInputTexture)));

    // Populate the subresource footprint, which describes how a flat buffer maps to the input texture
    mPlacedBufferFootprint.Footprint.Format = mDataFormat;
    mPlacedBufferFootprint.Footprint.Width = input_data.width;
    mPlacedBufferFootprint.Footprint.Height = input_data.height;
    mPlacedBufferFootprint.Footprint.Depth = 1;
    // Texture rows need to be aligned with 256 bytes(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT)
    mPlacedBufferFootprint.Footprint.RowPitch = std::ceil(sizeof(data_t) * (float)input_data.width / D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) * D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;

    // Create a flat buffer on the upload heap to upload the data into the GPU
    D3D12_RESOURCE_DESC buffer_description = CD3DX12_RESOURCE_DESC::Buffer(mPlacedBufferFootprint.Footprint.Height * mPlacedBufferFootprint.Footprint.RowPitch);
    ComPtr<ID3D12Resource> upload_buffer;
    D3D12_HEAP_PROPERTIES upload_heap = D3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    DirectXHelper::check_result(DirectXHelper::instance()->get_device()->CreateCommittedResource(&upload_heap, D3D12_HEAP_FLAG_NONE,
        &buffer_description, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&upload_buffer)));

    // Map the buffer for CPU access
    D3D12_RANGE read_range(0, 0); // We will only write the input data
    data_t* upload_buffer_start;
    DirectXHelper::check_result(upload_buffer->Map(0, &read_range, reinterpret_cast<void**>(&upload_buffer_start)));

    // Copy the input data into the upload buffer
    for (int y = 0; y < input_data.height; y++)
    {
        data_t* row_start = upload_buffer_start + y*mPlacedBufferFootprint.Footprint.RowPitch/sizeof(data_t);
        memcpy(row_start, &(input_data.data[y * input_data.width]), sizeof(data_t)*input_data.width);
    }

    // Copy the data from the upload buffer into the texture
    ComPtr<ID3D12GraphicsCommandList> command_list = DirectXHelper::instance()->create_direct_command_list();
    D3D12_TEXTURE_COPY_LOCATION destination = CD3DX12_TEXTURE_COPY_LOCATION(mInputTexture.Get());
    D3D12_TEXTURE_COPY_LOCATION source = CD3DX12_TEXTURE_COPY_LOCATION(upload_buffer.Get(), mPlacedBufferFootprint);
    command_list->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
    DirectXHelper::check_result(command_list->Close());

    // Execute the copy command and wait for it to finish, so that it doesn't affect benchmarking the algorithm
    DirectXHelper::instance()->execute_command_list_and_wait(command_list);

    // Create an UAV (unordered access view) to use the input texture in shaders
    D3D12_UNORDERED_ACCESS_VIEW_DESC unordered_access_view_desc{};
    unordered_access_view_desc.Format = texture_description.Format;
    unordered_access_view_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 0, mDescriptorSize);
    DirectXHelper::instance()->get_device()->CreateUnorderedAccessView(mInputTexture.Get(), nullptr, &unordered_access_view_desc, cpu_descriptor_handle);
}

void SummedAreaTableGeneratorGpuImpl::create_output_texture(const DataContainer& input_data)
{
    // Create the compute shader output texture 
    D3D12_RESOURCE_DESC texture_description{};
    texture_description.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    texture_description.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texture_description.Width = input_data.width;
    texture_description.Height = input_data.height;
    texture_description.MipLevels = 1;
    texture_description.DepthOrArraySize = 1;
    texture_description.SampleDesc.Count = 1;
    texture_description.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texture_description.Format = mDataFormat;
    D3D12_HEAP_PROPERTIES default_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    DirectXHelper::check_result(DirectXHelper::instance()->get_device()->CreateCommittedResource(&default_heap, D3D12_HEAP_FLAG_NONE,
        &texture_description, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&mOutputTexture)));

    // Create a readback buffer to read the output data back to the CPU
    D3D12_RESOURCE_DESC buffer_description = CD3DX12_RESOURCE_DESC::Buffer(mPlacedBufferFootprint.Footprint.Height * mPlacedBufferFootprint.Footprint.RowPitch);
    D3D12_HEAP_PROPERTIES readback_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
    DirectXHelper::check_result(DirectXHelper::instance()->get_device()->CreateCommittedResource(&readback_heap, D3D12_HEAP_FLAG_NONE,
        &buffer_description, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&mReadbackBuffer)));

    // Create an UAV (unordered access view) to use the output texture in shaders
    D3D12_UNORDERED_ACCESS_VIEW_DESC unordered_access_view_desc{};
    unordered_access_view_desc.Format = texture_description.Format;
    unordered_access_view_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 1, mDescriptorSize);
    DirectXHelper::instance()->get_device()->CreateUnorderedAccessView(mOutputTexture.Get(), nullptr, &unordered_access_view_desc, cpu_descriptor_handle);
}

void SummedAreaTableGeneratorGpuImpl::compute_summed_area_table(const DataContainer& input_data)
{
    ID3D12DescriptorHeap* descriptor_heaps[] = { mDescriptorHeap.Get() };

    // Dispatch the horizontal sweep shader to compute the horizontal sums of the summed area table
    ComPtr<ID3D12GraphicsCommandList> horizontal_command_list = DirectXHelper::instance()->create_direct_command_list();
    horizontal_command_list->SetDescriptorHeaps(1, descriptor_heaps);
    horizontal_command_list->SetComputeRootSignature(mHorizontalSweepShaderProgram.root_signature.Get());
    horizontal_command_list->SetComputeRootDescriptorTable(0, CD3DX12_GPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), 0, mDescriptorSize));
    horizontal_command_list->SetComputeRoot32BitConstant(1, DATA_MAX_VALUE, 0);
    horizontal_command_list->SetPipelineState(mHorizontalSweepShaderProgram.pipeline_state.Get());
    horizontal_command_list->Dispatch(1, std::ceil(input_data.height / THREAD_GROUP_SIZE), 1);

    // Add a barrier to ensure the horizontal sweep is finished 
    // before the vertical sweep to avoid a race condition
    D3D12_RESOURCE_BARRIER barrier[1] = {};
    barrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier[0].UAV.pResource = mOutputTexture.Get();
    horizontal_command_list->ResourceBarrier(_countof(barrier), &barrier[0]);
    DirectXHelper::check_result(horizontal_command_list->Close());

    ID3D12CommandList* command_lists[] = { horizontal_command_list.Get() };
    DirectXHelper::instance()->get_command_queue()->ExecuteCommandLists(1, command_lists);

    // Dispatch the vertical sweep shader to compute the vertical sums from the horizontal sums,
    // completing the summed area table
    ComPtr<ID3D12GraphicsCommandList> vertical_command_list = DirectXHelper::instance()->create_direct_command_list();

    vertical_command_list->SetDescriptorHeaps(1, descriptor_heaps);
    vertical_command_list->SetComputeRootSignature(mVerticalSweepShaderProgram.root_signature.Get());
    vertical_command_list->SetComputeRootDescriptorTable(0, CD3DX12_GPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), 0, mDescriptorSize));
    vertical_command_list->SetComputeRoot32BitConstant(1, DATA_MAX_VALUE, 0);
    vertical_command_list->SetPipelineState(mVerticalSweepShaderProgram.pipeline_state.Get());
    vertical_command_list->Dispatch(std::ceil(input_data.width / THREAD_GROUP_SIZE), 1, 1);
    DirectXHelper::check_result(vertical_command_list->Close());

    DirectXHelper::instance()->execute_command_list_and_wait(vertical_command_list);
}

void SummedAreaTableGeneratorGpuImpl::readback_output_data(const DataContainer& input_data, DataContainer& output_data)
{
    ComPtr<ID3D12GraphicsCommandList> command_list = DirectXHelper::instance()->create_direct_command_list();

    // Make the output texture copyable
    D3D12_RESOURCE_BARRIER barrier[1] = {};
    barrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier[0].Transition.pResource = mOutputTexture.Get();
    barrier[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier[0].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    command_list->ResourceBarrier(_countof(barrier), &barrier[0]);

    // Copy the output texture into the readback buffer
    D3D12_TEXTURE_COPY_LOCATION destination = CD3DX12_TEXTURE_COPY_LOCATION(mReadbackBuffer.Get(), mPlacedBufferFootprint);
    D3D12_TEXTURE_COPY_LOCATION source = CD3DX12_TEXTURE_COPY_LOCATION(mOutputTexture.Get());
    command_list->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);

    DirectXHelper::check_result(command_list->Close());
    ID3D12CommandList* command_lists[] = { command_list.Get() };
    DirectXHelper::instance()->execute_command_list_and_wait(command_list);

    // Map the readback buffer for CPU access
    data_t* readback_data_start;
    DirectXHelper::check_result(mReadbackBuffer->Map(0, nullptr, reinterpret_cast<void**>(&readback_data_start)));

    // Prepare the output data container
    output_data.width = input_data.width;
    output_data.height = input_data.height;
    output_data.data.resize(input_data.data.size());

    // Copy the data from the readback buffer into the output data container 
    for (int y = 0; y < input_data.height; y++)
    {
        data_t* row_start = readback_data_start + y * mPlacedBufferFootprint.Footprint.RowPitch / sizeof(data_t);
        memcpy(&(output_data.data[y*input_data.width]), row_start, sizeof(data_t) * input_data.width);
    }
}

void SummedAreaTableGeneratorGpuImpl::setup_shaders()
{
    // We can reuse the same root signature for both shaders
    ComPtr<ID3D12RootSignature> root_signature = create_root_signature();

    // Set up the horizontal sweep shader
    mHorizontalSweepShaderProgram.shader = DirectXHelper::instance()->compile_shader("summed_area_table_horizontal.compute.hlsl");
    mHorizontalSweepShaderProgram.root_signature = root_signature;
    setup_pipeline_state(mHorizontalSweepShaderProgram);

    // Set up the vertical sweep shader
    mVerticalSweepShaderProgram.shader = DirectXHelper::instance()->compile_shader("summed_area_table_vertical.compute.hlsl");
    mVerticalSweepShaderProgram.root_signature = root_signature;
    setup_pipeline_state(mVerticalSweepShaderProgram);

    // Create a descriptor heap for the compute shaders
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};
    heap_desc.NumDescriptors = 2;
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    DirectXHelper::check_result(DirectXHelper::instance()->get_device()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&mDescriptorHeap)));
    mDescriptorSize = DirectXHelper::instance()->get_device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void SummedAreaTableGeneratorGpuImpl::setup_pipeline_state(ShaderProgram& shader_program)
{
    D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline_state_desc{};
    pipeline_state_desc.pRootSignature = shader_program.root_signature.Get();
    pipeline_state_desc.CS = CD3DX12_SHADER_BYTECODE(shader_program.shader.Get());
    pipeline_state_desc.NodeMask = 0;
    DirectXHelper::check_result(DirectXHelper::instance()->get_device()->CreateComputePipelineState(&pipeline_state_desc, IID_PPV_ARGS(&(shader_program.pipeline_state))));
}

ComPtr<ID3D12RootSignature> SummedAreaTableGeneratorGpuImpl::create_root_signature()
{
    // Create root signature description
    const CD3DX12_STATIC_SAMPLER_DESC sampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
    CD3DX12_DESCRIPTOR_RANGE1 descriptor_range[1];
    descriptor_range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2, 0, 0);

    CD3DX12_ROOT_PARAMETER1 root_parameters[2];
    root_parameters[0].InitAsDescriptorTable(1, &descriptor_range[0]);
    root_parameters[1].InitAsConstants(1, 1);
    root_signature_desc.Init_1_1(_countof(root_parameters), root_parameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_NONE);

    // Serialize the root signature
    ComPtr<ID3DBlob> root_signature_blob;
    ComPtr<ID3DBlob> root_signature_error;
    DirectXHelper::check_result(D3DX12SerializeVersionedRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &root_signature_blob, &root_signature_error));

    if (!root_signature_error == NULL)
    {
        throw std::runtime_error("Failed to create root signature: " + std::string((char*)root_signature_error->GetBufferPointer()));
    }

    ComPtr<ID3D12RootSignature> root_signature;

    // Create the actual root signature
    DirectXHelper::check_result(DirectXHelper::instance()->get_device()->
        CreateRootSignature(0, root_signature_blob->GetBufferPointer(), root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&root_signature)));

    return root_signature;
}

