\# About

This is a programming assignment to compute a summed-area table (https://en.wikipedia.org/wiki/Summed-area\_table) using DirectX compute shaders.



\# Building 

Build by using CMake. DirectX SDK needs to be installed, obviously.
Tested with

'''
cmake -G "Visual Studio 17 2022"

'''
and then building the generated Visual Studio solution



\# Using the program

The program will take the following arguments:

'''
-shader\_dir or -s : Path to the shaders directory relative to the program
-file or -f: Path to the input text file relative to the program
-help or -h: Print documentation to the console

'''



The program default values will expect the program to be run in the
root directory containing the data and shaders directories. Here 

are the commands to run the included data files from the root directory:

'''
./SummedAreaTableUtility.exe -f twos\_128\_x\_128.txt
./SummedAreaTableUtility.exe -f twos\_256\_x\_256.txt
./SummedAreaTableUtility.exe -f twos\_1024\_x\_1024.txt
./SummedAreaTableUtility.exe -f twos\_2048\_x\_2048.txt
./SummedAreaTableUtility.exe -f data/descending\_10\_x\_10.txt
./SummedAreaTableUtility.exe -f data/large\_values\_10\_x\_10.txt
./SummedAreaTableUtility.exe -f data/ones\_10\_x\_10.txt
./SummedAreaTableUtility.exe -f data/ones\_12\_x\_13.txt
./SummedAreaTableUtility.exe -f data/ones\_2000\_x\_1000.txt
./SummedAreaTableUtility.exe -f data/square\_10\_x\_10.txt (the default input)

'''



Supports 8, 16 and 32 bit unsigned integers by changing data\_t
in constants.h. This can be tested with large\_values\_10\_x\_10.txt.

