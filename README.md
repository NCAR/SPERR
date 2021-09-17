[![clang-format](https://github.com/shaomeng/SPECK2020/actions/workflows/clang-format.yml/badge.svg)](https://github.com/shaomeng/SPECK2020/actions/workflows/clang-format.yml)
[![clang-tidy-qz](https://github.com/shaomeng/SPECK2020/actions/workflows/clang-tidy-qz.yml/badge.svg)](https://github.com/shaomeng/SPECK2020/actions/workflows/clang-tidy-qz.yml)
[![clang-tidy-size](https://github.com/shaomeng/SPECK2020/actions/workflows/clang-tidy-size.yml/badge.svg)](https://github.com/shaomeng/SPECK2020/actions/workflows/clang-tidy-size.yml)
[![cmake-release-qz](https://github.com/shaomeng/SPECK2020/actions/workflows/cmake-release-qz.yml/badge.svg)](https://github.com/shaomeng/SPECK2020/actions/workflows/cmake-release-qz.yml)
[![cmake-release-size](https://github.com/shaomeng/SPECK2020/actions/workflows/cmake-release-size.yml/badge.svg)](https://github.com/shaomeng/SPECK2020/actions/workflows/cmake-release-size.yml)

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

## Naming Convention on Memory Access Methods
For methods managing memory access of big chunks of memory, the following naming convention applies.
Notice the use of `const` and rvalue references, and choice of verbs in method names.
### Input
1. Make a copy of the input data block: 
- `auto copy_something(const T* buf, size_t len) -> status`
- `auto copy_something(const std::vector<T>& buf) -> status`
2. Take the ownership of the input data block:
- `auto take_something(std::vector<T>&& buf) -> status`
- `auto take_something(std::unique_ptr<T[]>&& buf, size_t len) -> status`

### Output
1. Provide read-only access to a block of memory:
- `auto view_something() const -> const std::vector<T>&`
- `auto view_something() const -> const T*`
2. Provide a copy of a block of memory that the class holds:
- `auto get_something() const -> std::vector<T>`
- `auto get_something() const -> std::unique_ptr<T[]>`
3. Release ownership of a block of memory that the class holds:
- `auto release_something() -> std::vector<T>&&`
- `auto release_something() -> std::unique_ptr<T[]>`

## When to use raw pointers?
This project is *strongly* against using raw pointers in the codebase. 
However, there are a few exceptional cases where raw pointers make a lot more
sense and they can be used.
1. When SPERR interacts with other projects and they might give SPERR a raw pointer.
- Example: when SPERR takes in input which is expressed as a raw pointer.
2. When SPERR *has* to use C interfaces that use raw pointers.
- Example: when SPERR calls ZSTD compression routines.
3. When doing type punning stuff.
- Example: when SPERR puts a double of 8 bytes to an arbitrary position in an outptu stream.
