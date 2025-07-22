# CMakeList.txt : CMake project for SummedAreaTable, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("SummedAreaTable")

# Add source to this project's executable.
add_executable (SummedAreaTable "main.cpp" "SummedAreaTableGeneratorCpuImpl.cpp" "SummedAreaTableGenerator.h"  "DataContainer.h" "InputParser.h" "InputParser.cpp" "SummedAreaTableGeneratorCpuImpl.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET SummedAreaTable PROPERTY CXX_STANDARD 20)
endif()
