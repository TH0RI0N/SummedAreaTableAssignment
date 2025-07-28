# About

This is a programming assignment to compute a summed-area table (https://en.wikipedia.org/wiki/Summed-area_table) using DirectX compute shaders.

# Building 

Build by using CMake. DirectX SDK needs to be installed, obviously. Tested with

```
cmake -G "Visual Studio 17 2022"

```
and then building the generated Visual Studio solution.



# Using the program

The program will take the following arguments:

```
-shader_dir or -s : Path to the shaders directory relative to the program
-file or -f: Path to the input text file relative to the program
-help or -h: Print documentation to the console

```

The program default values will expect the program to be run in the root directory containing the data and shaders directories. Here are the commands to run the included data files from the root directory:

```
./SummedAreaTableUtility.exe -f data/twos_128_x_128.txt
./SummedAreaTableUtility.exe -f data/twos_256_x_256.txt
./SummedAreaTableUtility.exe -f data/twos_1024_x_1024.txt
./SummedAreaTableUtility.exe -f data/twos_2048_x_2048.txt
./SummedAreaTableUtility.exe -f data/descending_10_x_10.txt
./SummedAreaTableUtility.exe -f data/large_values_10_x_10.txt
./SummedAreaTableUtility.exe -f data/ones_10_x_10.txt
./SummedAreaTableUtility.exe -f data/ones_12_x_13.txt
./SummedAreaTableUtility.exe -f data/ones_2000_x_1000.txt
./SummedAreaTableUtility.exe -f data/square_10_x_10.txt (the default input)

```

Supports 8, 16 and 32 bit unsigned integers by changing data_t
in constants.h. This can be tested with large_values_10_x_10.txt.
