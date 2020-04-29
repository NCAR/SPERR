# SPECK2020
Attempt in 2019-2020 to port SPECK algorithm for VAPOR.

## Build
SPECK2020 uses `cmake` to build. It does require cmake version higher than `3.10`.
After downloading the source code in directory `SPECK2020`, follow the steps below
to have it built on a UNIX system.

```
cd SPECK2020
mkdir build
cmake ..
make -j 4
```

## Executables
After building it, there will be 6 executable in the current directory:
4 examples (`example_xxx`), 1 compressor (`compressor_3d`),
and 1 decompressor (`compressor_3d`).
The examples showcase how to use key functions and to perform various tasks,
while the compressor and decompressor could be used in production settings.

## Optional Configurations
A user could use `ccmake` to toggle optional configurations.
1. `BUILD_UNIT_TESTS`: If set to be `ON`, then a set of unit tests are built.
2. `TIME_EXAMPLES`: If set to be `ON`, then some of the executables will print
out execution time numbers.
