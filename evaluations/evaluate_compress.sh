#!/bin/bash

bpp=(0.25 0.5 1 2 4)

for t in ${bpp[*]}; do
    ../build/compressor_3d ../test_data/wmag128.float 128 128 128 XYZ tmp $t >> 128_cube.result
done

for t in ${bpp[*]}; do
    ../build/compressor_3d ../test_data/wmag256.float 256 256 256 XYZ tmp $t >> 256_cube.result
done

for t in ${bpp[*]}; do
    ../build/compressor_3d ../test_data/wmag512.float 512 512 512 XYZ tmp $t >> 512_cube.result
done

# for t in ${bpp[*]}; do
#     ../build/decompressor_3d ../build/128_bpp_5.bitstream $t tmp >> 128_cube.result
# done
# 
# for t in ${bpp[*]}; do
#     ../build/decompressor_3d ../build/256_bpp_5.bitstream $t tmp >> 256_cube.result
# done
# 
# for t in ${bpp[*]}; do
#     ../build/decompressor_3d ../build/512_bpp_5.bitstream $t tmp >> 512_cube.result
# done
