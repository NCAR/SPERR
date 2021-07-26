[![clang-format Check](https://github.com/shaomeng/SPECK2020/actions/workflows/clang-format-check.yml/badge.svg)](https://github.com/shaomeng/SPECK2020/actions/workflows/clang-format-check.yml)
[![cmake-release-qz](https://github.com/shaomeng/SPECK2020/actions/workflows/cmake-release-qz.yml/badge.svg)](https://github.com/shaomeng/SPECK2020/actions/workflows/cmake-release-qz.yml)
[![cmake-release-qz](https://github.com/shaomeng/SPECK2020/actions/workflows/cmake-release-qz.yml/badge.svg)](https://github.com/shaomeng/SPECK2020/actions/workflows/cmake-release-qz.yml)

# SPECK2020
Attempt in 2019-2020 to port SPECK algorithm for VAPOR.

## Build
SPECK2020 uses `cmake` to build. It does require cmake version higher than `3.10`.
After downloading the source code in directory `SPECK2020`, follow the steps below
to have it built on a UNIX system.

```
cd SPECK2020
mkdir build
cd build
cmake ..
make -j 4
```

## Default Executables
After building it, there will be 3 executable in the current directory:
`compressor_3d`, `decompressor_3d`, and `probe_3d`.
While the compressor and decompressor are supposed to be used in production,
the probe is supposed to provide richer information (PSNR, L-Infty, etc.) 
on how well SPERR works on a particular data with a particular setting. 

## Optional Configurations
A user could use `ccmake` to toggle optional configurations.
1. `BUILD_UNIT_TESTS`: If set to be `ON`, then a set of unit tests are built.

2. `QZ_TERM`: Determines the algorithm termination criterion. If set `OFF`, then
the encoding algorithm will terminate when a user-defined storage budget is met.
If set `On`, then the encoding algorithm will terminate after processing a user-defined 
quantization level. In this case, the output size is non-deterministic.

3. `USE_ZSTD`. Determines if the SPECK output bitstream further goes through an [ZSTD] compression.
Experiments show that this step can further reduce the output size by ~ 5% without obvious
performance impact.

