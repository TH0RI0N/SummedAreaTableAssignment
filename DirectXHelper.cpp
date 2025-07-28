#include "DirectXHelper.h"

#include <stdexcept>
#include <functional>
#include <format>
#include <iostream>
#include <filesystem>

#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include "d3dx12.h"

std::unique_ptr<DirectXHelper> DirectXHelper::mInstance = nullptr;

void DirectXHelper::init(const std::string& shader_directory_path)
{
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
    }
    if (mInstance != nullptr)
    {
        return;
    }

    mInstance = std::unique_ptr<DirectXHelper>(new DirectXHelper());
    mInstance->init_d3d_device();
    mInstance->init_interfaces();
    if (!shader_directory_path.empty())
    {
        mInstance->set_shader_directory(shader_directory_path);
    }
}

void DirectXHelper::check_result(HRESULT result)
{
    if (FAILED(result))
    {
        std::cout << "Failed DirectX HRESULT 0x" << std::hex << result << std::endl;
        throw std::runtime_error("Critical failure");
    }
}

DirectXHelper* DirectXHelper::instance()
{
	if (mInstance == nullptr)
	{
        init();
	}
	return mInstance.get();
}

void DirectXHelper::init_d3d_device()
{
    ComPtr<IDXGIFactory2> factory;
    CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));

    // Create an enumerator for hardware adapters, trying to find a high performance (non-integrated) GPU if possible
    ComPtr<IDXGIFactory6> factory6;
    ComPtr<IDXGIAdapter1> hardware_adapter;
    std::function<bool(UINT)> adapter_enumerator;
    if (SUCCEEDED(factory->QueryInterface(IID_PPV_ARGS(&factory6))))
    {
        adapter_enumerator = [&](UINT index) {return SUCCEEDED(factory6->EnumAdapterByGpuPreference(index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&hardware_adapter)));};
    }
    else
    {
        adapter_enumerator = [&](UINT index) {return SUCCEEDED(factory->EnumAdapters1(index, &hardware_adapter));};
    }

    // Enumerate through hardware adapters to find one that supports Direct3D 12
    for (UINT adapter_index = 0; adapter_enumerator(adapter_index); ++adapter_index)
    {
        DXGI_ADAPTER_DESC1 desc;
        hardware_adapter->GetDesc1(&desc);

        // Ignore possible software renderers
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            continue;
        }

        // Check to see whether the adapter supports Direct3D 12
        if (SUCCEEDED(D3D12CreateDevice(hardware_adapter.Get(), D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr)))
        {
            break;
        }
    }

    if (hardware_adapter.Get() == nullptr)
    {
        throw std::runtime_error("GPU not found");
    }

    // Create Direct3D device to interface with
    check_result(D3D12CreateDevice(hardware_adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mDevice)));
}

void DirectXHelper::init_interfaces()
{
    D3D12_COMMAND_QUEUE_DESC queue_desc = D3D12_COMMAND_QUEUE_DESC();
    check_result(mDevice->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&mCommandQueue)));

    D3D12_COMMAND_LIST_TYPE command_list_type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    check_result(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mDirectCommandAllocator)));

    check_result(get_device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
}

ID3D12Device2* DirectXHelper::get_device()
{
    return mDevice.Get();
}

ID3D12CommandQueue* DirectXHelper::get_command_queue()
{
    return mCommandQueue.Get();
}

ID3D12CommandAllocator* DirectXHelper::get_direct_command_allocator()
{
    return mDirectCommandAllocator.Get();
}

ComPtr<ID3D12GraphicsCommandList> DirectXHelper::create_direct_command_list()
{
    ComPtr<ID3D12GraphicsCommandList> command_list_out;
    check_result(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mDirectCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&command_list_out)));
    return command_list_out;
}

void DirectXHelper::execute_command_list_and_wait(ComPtr<ID3D12GraphicsCommandList> command_list)
{
    ID3D12CommandList* command_lists[] = { command_list.Get() };
    get_command_queue()->ExecuteCommandLists(1, command_lists);

    check_result(get_command_queue()->Signal(mFence.Get(), mFenceValue));
    HANDLE handle = CreateEvent(NULL, FALSE, FALSE, NULL);
    mFence->SetEventOnCompletion(mFenceValue, handle);
    WaitForSingleObject(handle, INFINITE);
    // Use 2 different fence values, so we can reuse the fence
    mFenceValue = mFenceValue == 1 ? 2 : 1;
}

ComPtr<ID3DBlob> DirectXHelper::compile_shader(const std::string& shader_file_path)
{
    std::string total_path = mShaderDirectoryPath + shader_file_path;
    if (!std::filesystem::exists(total_path))
    {
        throw std::runtime_error("Could not find shader file: " + total_path);
    }

    ComPtr<ID3DBlob> shader;
	ComPtr<ID3DBlob> compile_error;
    std::wstring path_wstring(total_path.begin(), total_path.end());
	D3DCompileFromFile(path_wstring.c_str(), NULL, NULL, "main", "cs_5_1", 0, 0, &shader, &compile_error);

	if (compile_error != NULL)
	{
		std::string error_msg = "Shader compilation failed: ";
		error_msg += std::string((char*)compile_error->GetBufferPointer());
		throw std::runtime_error(error_msg);
	}

    return shader;
}

void DirectXHelper::set_shader_directory(const std::string& directory_path)
{
    if (!directory_path.empty() && directory_path[directory_path.size() - 1] != '/')
    {
        mShaderDirectoryPath = directory_path + '/';
    }
    else
    {
        mShaderDirectoryPath = directory_path;
    }
}
