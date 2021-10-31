[![clang-format](https://github.com/shaomeng/SPECK2020/actions/workflows/clang-format.yml/badge.svg)](https://github.com/shaomeng/SPECK2020/actions/workflows/clang-format.yml)
[![clang-tidy-qz](https://github.com/shaomeng/SPECK2020/actions/workflows/clang-tidy-qz.yml/badge.svg)](https://github.com/shaomeng/SPECK2020/actions/workflows/clang-tidy-qz.yml)
[![clang-tidy-size](https://github.com/shaomeng/SPECK2020/actions/workflows/clang-tidy-size.yml/badge.svg)](https://github.com/shaomeng/SPECK2020/actions/workflows/clang-tidy-size.yml)
[![unit-test-qz](https://github.com/shaomeng/SPECK2020/actions/workflows/unit-test-qz.yml/badge.svg)](https://github.com/shaomeng/SPECK2020/actions/workflows/unit-test-qz.yml)
[![unit-test-size](https://github.com/shaomeng/SPECK2020/actions/workflows/unit-test-size.yml/badge.svg)](https://github.com/shaomeng/SPECK2020/actions/workflows/unit-test-size.yml)
[![CodeQL](https://github.com/shaomeng/SPERR/actions/workflows/codeql-analysis.yml/badge.svg?branch=main)](https://github.com/shaomeng/SPERR/actions/workflows/codeql-analysis.yml)


This repository hosts the source code for SPERR.
More introduction and documentation about SPERR can be found on its main
webpate: https://shaomeng.github.io/SPERR/.

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
sense and they can be used. Overall, the use of raw pointers needs to be looked
at on a case-by-case basis, and most of the time, an alternative is preferred.
1. When SPERR interacts with other projects and they might give SPERR a raw pointer.
- Example: when SPERR takes in input which is expressed as a raw pointer.
2. When SPERR *has* to use C interfaces that use raw pointers.
- Example: when SPERR calls ZSTD compression routines.
3. When doing type punning stuff.
- Example: when SPERR puts a double of 8 bytes to an arbitrary position in an outptu stream.

### What are the alternatives?
If I want to specify a function to process a specific range of a long array, what are
my options?
1. Iterators. This is the convention of modern C++.
2. A reference to the array and then an offset value.

Note: `src/CDF97.h/cpp` provide some good examples.
