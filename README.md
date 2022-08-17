[![clang-format](https://github.com/NCAR/SPERR/actions/workflows/clang-format.yml/badge.svg)](https://github.com/NCAR/SPERR/actions/workflows/clang-format.yml)
[![clang-tidy](https://github.com/NCAR/SPERR/actions/workflows/clang-tidy.yml/badge.svg)](https://github.com/NCAR/SPERR/actions/workflows/clang-tidy.yml)
[![unit-test](https://github.com/NCAR/SPERR/actions/workflows/unit-test.yml/badge.svg)](https://github.com/NCAR/SPERR/actions/workflows/unit-test.yml)
[![CodeQL](https://github.com/NCAR/SPERR/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/NCAR/SPERR/actions/workflows/codeql-analysis.yml)

## Overview

SPERR is a lossy compressor for scientific data (2D or 3D floating-point data, mostly produced by numerical simulations). 
SPERR produces excellent rate-distortion curves, meaning that it achieves the least amount of ***average error***
given a certain storage budget.

Under the hood, SPERR uses wavelet transforms, [SPECK](https://ieeexplore.ieee.org/document/1347192) coding, 
and a custom outlier coding algorithm in its compression pipeline. 
This combination gives SPERR flexibility to compress targetting different quality controls, namely 1) bit-per-pixel (BPP), 
2) peak signal-to-noise ratio (PSNR), and 3) point-wise error (PWE).
The name of SPERR stands for **SP**eck with **ERR**or bounding.

## Documentation

SPERR documentation is hosted on Github [Wiki](https://github.com/shaomeng/SPERR/wiki) pages. To get started, one might want to
[build SPERR from source](https://github.com/shaomeng/SPERR/wiki/Build-SPERR-From-Source) and explore [compression and decompression
utilities](https://github.com/shaomeng/SPERR/wiki/CLI:-Compression-And-Decompression-Utilities). 

SPERR also provides programming [API in C++ and C](https://github.com/shaomeng/SPERR/wiki#sperr-c-api). 

