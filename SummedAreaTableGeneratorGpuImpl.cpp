#include "SummedAreaTableGeneratorGpuImpl.h"

#include "constants.h"
#include "DirectXHelper.h"
#include "d3dx12.h"

#include <chrono>
#include <iostream>
#include <thread>

static const std::string SHADER_DIRECTORY_PATH = "shaders/";

SummedAreaTableGeneratorGpuImpl::SummedAreaTableGeneratorGpuImpl()
{
    mCommandList = DirectXHelper::instance()->create_direct_command_list();

    std::cout << "Compiling shaders..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    setup_horizontal_sweep_shader();
    setup_vertical_sweep_shader();

    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Shaders compiled in "
        << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
        << "ms" << std::endl;

    // Create a descriptor heap for the compute shaders

    D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};
    heap_desc.NumDescriptors = 2;
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    DirectXHelper::check_result(DirectXHelper::instance()->get_device()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&mDescriptorHeap)));
    mDescriptorSize = DirectXHelper::instance()->get_device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    ID3D12DescriptorHeap* descriptor_heaps[] = { mDescriptorHeap.Get() };
    mCommandList->SetDescriptorHeaps(1, descriptor_heaps);
}

int SummedAreaTableGeneratorGpuImpl::generate(const DataContainer& data_in, DataContainer& data_out)
{
    create_input_texture(data_in);
    create_output_texture(data_in);

    // Execute the commands to populate the input texture with the input data (and wait 
    // for them to finish, so that it doesn't interfere with benchmarking the algorithm itself)
    //DirectXHelper::instance()->execute_command_list_and_wait(mCommandList);

    auto start = std::chrono::high_resolution_clock::now();
    compute_summed_area_table(data_in);
    auto end = std::chrono::high_resolution_clock::now();

    readback_output_data(data_in, data_out);

    return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}

void SummedAreaTableGeneratorGpuImpl::create_input_texture(const DataContainer& input_data)
{
    static_assert(sizeof(data_t) == sizeof(uint8_t) && "Changing the data size needs to be accounted for in this method!");

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
    texture_description.Format = DXGI_FORMAT_R8_UINT;
    D3D12_HEAP_PROPERTIES default_heap = D3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    DirectXHelper::check_result(DirectXHelper::instance()->get_device()->CreateCommittedResource(&default_heap, D3D12_HEAP_FLAG_NONE,
        &texture_description, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&mInputTexture)));

    // Create a buffer on the upload heap to upload the data into the GPU
    D3D12_RESOURCE_DESC buffer_description = CD3DX12_RESOURCE_DESC::Buffer(sizeof(data_t) * input_data.data.size());
    ComPtr<ID3D12Resource> upload_buffer;
    D3D12_HEAP_PROPERTIES upload_heap = D3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    DirectXHelper::check_result(DirectXHelper::instance()->get_device()->CreateCommittedResource(&upload_heap, D3D12_HEAP_FLAG_NONE,
        &buffer_description, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&upload_buffer)));

    // Map the buffer for CPU access
    D3D12_RANGE read_range(0, 0); // We will only write the input data
    data_t* upload_buffer_data;
    DirectXHelper::check_result(upload_buffer->Map(0, &read_range, reinterpret_cast<void**>(&upload_buffer_data)));

    // Copy the input data into the upload buffer
    memcpy(upload_buffer_data, &(input_data.data[0]), sizeof(data_t)*input_data.data.size());
    //int row_pitch = std::ceil(sizeof(data_t) * (float)input_data.width / D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) * D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
    /*for (data_t y = 0; y < input_data.height; y++)
    {
        data_t* row_start = texture_data + y*row_pitch;
        memcpy(row_start, &(input_data.data[y * input_data.width]), sizeof(data_t) * input_data.width);
    }*/

    // Add a command to copy the data from the buffer into the texture
    D3D12_TEXTURE_COPY_LOCATION destination = CD3DX12_TEXTURE_COPY_LOCATION(mInputTexture.Get());
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT placed_footprint{};
    placed_footprint.Footprint.Format = DXGI_FORMAT_R8_UINT;
    placed_footprint.Footprint.Width = input_data.width;
    placed_footprint.Footprint.Height = input_data.height;
    placed_footprint.Footprint.Depth = 1;
    placed_footprint.Footprint.RowPitch = 256;
    D3D12_TEXTURE_COPY_LOCATION source = CD3DX12_TEXTURE_COPY_LOCATION(upload_buffer.Get(), placed_footprint);
    mCommandList->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);

    mCommandList->Close();

    DirectXHelper::instance()->execute_command_list_and_wait(mCommandList);
    mCommandList = DirectXHelper::instance()->create_direct_command_list();

    // Create an UAV (unordered access view) to use this texture in shaders
    D3D12_UNORDERED_ACCESS_VIEW_DESC unordered_access_view_desc{};
    unordered_access_view_desc.Format = texture_description.Format;
    unordered_access_view_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 0, mDescriptorSize);
    DirectXHelper::instance()->get_device()->CreateUnorderedAccessView(mInputTexture.Get(), nullptr, &unordered_access_view_desc, cpu_descriptor_handle);
}

void SummedAreaTableGeneratorGpuImpl::create_output_texture(const DataContainer& input_data)
{
    static_assert(sizeof(data_t) == sizeof(uint8_t) && "Changing the data size needs to be accounted for in this method!");

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
    texture_description.Format = DXGI_FORMAT_R8_UINT;
    D3D12_HEAP_PROPERTIES default_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    DirectXHelper::check_result(DirectXHelper::instance()->get_device()->CreateCommittedResource(&default_heap, D3D12_HEAP_FLAG_NONE,
        &texture_description, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&mOutputTexture)));

    // Create a readback buffer
    D3D12_RESOURCE_DESC buffer_description = CD3DX12_RESOURCE_DESC::Buffer(sizeof(data_t) * input_data.data.size());
    D3D12_HEAP_PROPERTIES readback_heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
    DirectXHelper::check_result(DirectXHelper::instance()->get_device()->CreateCommittedResource(&readback_heap, D3D12_HEAP_FLAG_NONE,
        &buffer_description, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&mReadbackBuffer)));

    DirectXHelper::check_result(mReadbackBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mReadbackData)));

    // Create an UAV (unordered access view) to use the output texture in shaders
    D3D12_UNORDERED_ACCESS_VIEW_DESC unordered_access_view_desc{};
    unordered_access_view_desc.Format = texture_description.Format;
    unordered_access_view_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 1, mDescriptorSize);
    DirectXHelper::instance()->get_device()->CreateUnorderedAccessView(mOutputTexture.Get(), nullptr, &unordered_access_view_desc, cpu_descriptor_handle);
}

void SummedAreaTableGeneratorGpuImpl::compute_summed_area_table(const DataContainer& input_data)
{
    /*/D3D12_TEXTURE_COPY_LOCATION destination = CD3DX12_TEXTURE_COPY_LOCATION(mOutputTexture.Get());
    D3D12_TEXTURE_COPY_LOCATION source = CD3DX12_TEXTURE_COPY_LOCATION(mInputTexture.Get());
    mCommandList->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);*/
    ID3D12DescriptorHeap* descriptor_heaps[] = { mDescriptorHeap.Get() };
    mCommandList->SetDescriptorHeaps(1, descriptor_heaps);
    mCommandList->SetComputeRootSignature(mHorizontalSweepShaderProgram.root_signature.Get());
    mCommandList->SetPipelineState(mHorizontalSweepShaderProgram.pipeline_state.Get());
    D3D12_GPU_DESCRIPTOR_HANDLE gpu_descriptor_handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), 0, mDescriptorSize);
    mCommandList->SetComputeRootDescriptorTable(0, gpu_descriptor_handle);

    unsigned int dispatchX = std::ceil(input_data.width / 32.0f);
    mCommandList->Dispatch(dispatchX, 1, 1);

    //DirectXHelper::instance()->execute_command_list_and_wait(mCommandList);
}

void SummedAreaTableGeneratorGpuImpl::readback_output_data(const DataContainer& input_data, DataContainer& output_data)
{
    // Prepare the output data container
    output_data.width = input_data.width;
    output_data.height = input_data.height;
    output_data.data.resize(input_data.data.size());

    // Make the output texture copyable
    D3D12_RESOURCE_BARRIER barrier[1] = {};
    barrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier[0].Transition.pResource = mOutputTexture.Get();
    barrier[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier[0].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    mCommandList->ResourceBarrier(_countof(barrier), &barrier[0]);

    // Copy the output texture into the readback buffer
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT placed_footprint{};
    placed_footprint.Footprint.Format = DXGI_FORMAT_R8_UINT;
    placed_footprint.Footprint.Width = input_data.width;
    placed_footprint.Footprint.Height = input_data.height;
    placed_footprint.Footprint.Depth = 1;
    placed_footprint.Footprint.RowPitch = 256;
    D3D12_TEXTURE_COPY_LOCATION destination = CD3DX12_TEXTURE_COPY_LOCATION(mReadbackBuffer.Get(), placed_footprint);
    D3D12_TEXTURE_COPY_LOCATION source = CD3DX12_TEXTURE_COPY_LOCATION(mOutputTexture.Get());
    mCommandList->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
    //mCommandList->CopyResource(mReadbackBuffer.Get(), mOutputTexture.Get());

    DirectXHelper::check_result(mCommandList->Close());

    DirectXHelper::instance()->execute_command_list_and_wait(mCommandList);

    // Copy the data from the readback buffer into the output data container
    memcpy(&(output_data.data[0]), mReadbackData, sizeof(data_t) * input_data.data.size());
}

void SummedAreaTableGeneratorGpuImpl::setup_horizontal_sweep_shader()
{
    mHorizontalSweepShaderProgram.shader = DirectXHelper::instance()->compile_shader(SHADER_DIRECTORY_PATH + "summed_area_table_horizontal.compute.hlsl");

    // Create root signature description
    const CD3DX12_STATIC_SAMPLER_DESC sampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
    CD3DX12_DESCRIPTOR_RANGE1 descriptor_range[2];
    descriptor_range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
    descriptor_range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, 0);

    CD3DX12_ROOT_PARAMETER1 root_parameters[1];
    root_parameters[0].InitAsDescriptorTable(2, &descriptor_range[0]);
    root_signature_desc.Init_1_1(_countof(root_parameters), root_parameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_NONE);

    // Serialize the root signature
    ComPtr<ID3DBlob> root_signature_blob;
    ComPtr<ID3DBlob> root_signature_error;
    DirectXHelper::check_result(D3DX12SerializeVersionedRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &root_signature_blob, &root_signature_error));

    if (!root_signature_error == NULL)
    {
        throw std::runtime_error("Failed to create root signature: " + std::string((char*)root_signature_error->GetBufferPointer()));
    }

    // Create the actual root signature
    DirectXHelper::check_result(DirectXHelper::instance()->get_device()->
        CreateRootSignature(0, root_signature_blob->GetBufferPointer(), root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&mHorizontalSweepShaderProgram.root_signature)));

    setup_pipeline_state(mHorizontalSweepShaderProgram);
}

void SummedAreaTableGeneratorGpuImpl::setup_vertical_sweep_shader()
{
    mVerticalSweepShaderProgram.shader = DirectXHelper::instance()->compile_shader(SHADER_DIRECTORY_PATH + "summed_area_table_vertical.compute.hlsl");

    // Create root signature description
    const CD3DX12_STATIC_SAMPLER_DESC sampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
    CD3DX12_DESCRIPTOR_RANGE1 descriptor_range[1];
    descriptor_range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, 0);

    CD3DX12_ROOT_PARAMETER1 root_parameters[1];
    root_parameters[0].InitAsDescriptorTable(1, &descriptor_range[0]);
    root_signature_desc.Init_1_1(_countof(root_parameters), root_parameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_NONE);

    // Serialize the root signature
    ComPtr<ID3DBlob> root_signature_blob;
    ComPtr<ID3DBlob> root_signature_error;
    DirectXHelper::check_result(D3DX12SerializeVersionedRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &root_signature_blob, &root_signature_error));

    if (!root_signature_error == NULL)
    {
        throw std::runtime_error("Failed to create root signature: " + std::string((char*)root_signature_error->GetBufferPointer()));
    }

    // Create the actual root signature
    DirectXHelper::check_result(DirectXHelper::instance()->get_device()->
        CreateRootSignature(0, root_signature_blob->GetBufferPointer(), root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&mVerticalSweepShaderProgram.root_signature)));

    setup_pipeline_state(mVerticalSweepShaderProgram);
}

void SummedAreaTableGeneratorGpuImpl::setup_pipeline_state(ShaderProgram& shader_program)
{
    D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline_state_desc{};
    pipeline_state_desc.pRootSignature = shader_program.root_signature.Get();
    pipeline_state_desc.CS = CD3DX12_SHADER_BYTECODE(shader_program.shader.Get());
    pipeline_state_desc.NodeMask = 0;
    DirectXHelper::check_result(DirectXHelper::instance()->get_device()->CreateComputePipelineState(&pipeline_state_desc, IID_PPV_ARGS(&(shader_program.pipeline_state))));
}

