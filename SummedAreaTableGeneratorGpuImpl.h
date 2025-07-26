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
	virtual int generate(const DataContainer& data_in, DataContainer& data_out) override;
private:
	struct ShaderProgram
	{
		ComPtr<ID3DBlob> shader;
		ComPtr<ID3D12RootSignature> root_signature;
		ComPtr<ID3D12PipelineState> pipeline_state;
	};

	void create_input_texture(const DataContainer& input_data);
	void create_output_texture(const DataContainer& input_data);
	void compute_summed_area_table(const DataContainer& input_data);
	void readback_output_data(const DataContainer& input_data, DataContainer& output_data);

	void setup_horizontal_sweep_shader();
	void setup_vertical_sweep_shader();
	void setup_pipeline_state(ShaderProgram& shader_program);

	ComPtr<ID3D12Resource> mInputTexture;
	ComPtr<ID3D12Resource> mOutputTexture;
	ComPtr<ID3D12Resource> mReadbackBuffer;
	data_t* mReadbackData = nullptr;

	ShaderProgram mHorizontalSweepShaderProgram;
	ShaderProgram mVerticalSweepShaderProgram;

	ComPtr<ID3D12GraphicsCommandList> mCommandList;
	ComPtr<ID3D12DescriptorHeap> mDescriptorHeap;
	unsigned int mDescriptorSize;
};