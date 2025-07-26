# CMakeList.txt : CMake project for SummedAreaTable, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# Add source to this project's executable.
add_executable (SummedAreaTable "main.cpp" "SummedAreaTableGeneratorCpuImpl.cpp" "SummedAreaTableGenerator.h"  "DataContainer.h" "InputParser.h" "InputParser.cpp" "SummedAreaTableGeneratorCpuImpl.h" "SummedAreaTableGeneratorGpuImpl.cpp" "constants.h" "DirectXHelper.h" "DirectXHelper.cpp" "d3dx12.h")

find_path(D3D12_INCLUDE_DIR    # Set variable D3D12_INCLUDE_DIR
          d3d12.h                # Find a path with d3d12.h
          HINTS "${WIN10_SDK_PATH}/Include/${WIN10_SDK_VERSION}/um"
          DOC "path to WIN10 SDK header files"
          HINTS
          )

find_path(DXGI_INCLUDE_DIR    # Set variable DXGI_INCLUDE_DIR
          dxgi1_4.h           # Find a path with dxgi1_4.h
          HINTS "${WIN10_SDK_PATH}/Include/${WIN10_SDK_VERSION}/shared"
          DOC "path to WIN10 SDK header files"
          HINTS
          )
set(D3D12_INCLUDE_DIRS ${D3D12_INCLUDE_DIR} ${DXGI_INCLUDE_DIR})

# List of D3D libraries
set(D3D12_LIBRARIES d3d12.lib dxgi.lib d3dcompiler.lib)

target_link_libraries(SummedAreaTable d3d12.lib dxgi.lib d3dcompiler.lib)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET SummedAreaTable PROPERTY CXX_STANDARD 20)
endif()
