# CMakeList.txt : CMake project for SummedAreaTableUtility
cmake_minimum_required (VERSION 3.8)

add_executable (SummedAreaTableUtility 
    "main.cpp"
    "constants.h"
    "d3dx12.h"
    "DirectXHelper.h"
    "DirectXHelper.cpp"
    "DataContainer.h"
    "InputParser.h" 
    "InputParser.cpp"
    "SummedAreaTableGenerator.h"
    "SummedAreaTableGeneratorCpuImpl.h"
    "SummedAreaTableGeneratorCpuImpl.cpp"
    "SummedAreaTableGeneratorGpuImpl.h"
    "SummedAreaTableGeneratorGpuImpl.cpp")

target_link_libraries(SummedAreaTableUtility d3d12.lib dxgi.lib d3dcompiler.lib)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET SummedAreaTableUtility PROPERTY CXX_STANDARD 20)
endif()
