#pragma once

#include <string>
#include <memory>

#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>

using namespace Microsoft::WRL;

// Singleton class for DirectX interaction not strictly related to
// the generation of the summed area table in this demo
class DirectXHelper
{
public:
	// Singleton not copyable or assignable
	DirectXHelper(DirectXHelper&) = delete;
	void operator=(const DirectXHelper&) = delete;

	// Throw an exception if the result is not successful
	static void check_result(HRESULT result);

	// Get the singleton instance of the DirectXHelper
	static DirectXHelper* instance();

	// Can be called perform the first time initialization.
	// This is automatically done when calling instance() for the first 
	// time, if init() is not called beforehand
	static void init();

	// Access the Direct3D device
	ID3D12Device2* get_device();

	// Access the Direct3D command queue
	ID3D12CommandQueue* get_command_queue();

	// Access the Direct3D command allocator for direct command lists (not bundles)
	ID3D12CommandAllocator* get_direct_command_allocator();

	// Create a new direct command list. The given pointer will point to the new list.
	ComPtr<ID3D12GraphicsCommandList> create_direct_command_list();

	// Execute the given command list and wait for it's completion
	void execute_command_list_and_wait(ComPtr<ID3D12GraphicsCommandList> command_list);

	// Compile the given shader file and prepare the root signature and pipeline state to use it
	ComPtr<ID3DBlob> compile_shader(std::string shader_file_path);
private:
	// Hide the constructor, so the singleton is only accessible through instance()
	DirectXHelper() = default;

	void init_d3d_device();

	// Initialize the offered Direct3D interfaces
	void init_interfaces();

	static std::unique_ptr<DirectXHelper> mInstance;
	ComPtr<ID3D12Device2> mDevice = nullptr;
	ComPtr<ID3D12CommandQueue> mCommandQueue = nullptr;
	ComPtr<ID3D12CommandAllocator> mDirectCommandAllocator = nullptr;
	ComPtr<ID3D12Fence> mFence = nullptr;
	int mFenceValue = 1;
};