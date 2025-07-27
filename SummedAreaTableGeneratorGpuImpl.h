#pragma once

#include "SummedAreaTableGenerator.h"
#include "DirectXHelper.h"

#include <wrl/client.h>
#include <d3d12.h>

using namespace Microsoft::WRL;

// Summed area table generator using the GPU
// https://en.wikipedia.org/wiki/Summed-area_table
// To be efficient using the GPU architecture, a different algorithm
// is needed compared to using the CPU. The problem is separable
// (We can calculate horizontal and vertical sums separately),
// so we will do that with compute shaders.
class SummedAreaTableGeneratorGpuImpl : public SummedAreaTableGenerator
{
public:
	SummedAreaTableGeneratorGpuImpl();
	virtual float generate(const DataContainer& data_in, DataContainer& data_out) override;
private:
	struct ShaderProgram
	{
		ComPtr<ID3DBlob> shader;
		ComPtr<ID3D12RootSignature> root_signature;
		ComPtr<ID3D12PipelineState> pipeline_state;
	};

	struct ComputeShaderConstants
	{
		int data_max_size;
	};

	void create_input_texture(const DataContainer& input_data);
	void create_output_texture(const DataContainer& input_data);
	void create_constant_buffer();
	void compute_summed_area_table(const DataContainer& input_data);
	void readback_output_data(const DataContainer& input_data, DataContainer& output_data);

	// Load and compile shaders and prepare the ShaderProgram structs
	void setup_shaders();

	// Create the pipeline state in the given struct from the other inputs.
	// Thus the shader and root_signature are needed before calling this method
	void setup_pipeline_state(ShaderProgram& shader_program);

	// We can reuse the same root signature for both shaders
	ComPtr<ID3D12RootSignature> create_root_signature();

	DXGI_FORMAT mDataFormat;
	ComPtr<ID3D12Resource> mInputTexture;
	ComPtr<ID3D12Resource> mOutputTexture;
	ComPtr<ID3D12Resource> mReadbackBuffer;
	ComPtr<ID3D12Resource> mConstantBuffer;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT mPlacedBufferFootprint{};
	ShaderProgram mHorizontalSweepShaderProgram;
	ShaderProgram mVerticalSweepShaderProgram;
	ComPtr<ID3D12DescriptorHeap> mDescriptorHeap;
	unsigned int mDescriptorSize;
};