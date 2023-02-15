#!/bin/bash

# create a few symbolic links
FILE=bin
if test -e "$FILE"; then
  echo "--> C++ bin dir exists"
else
  ln -s ../../install/bin .
fi

FILE=test_data
if test -e "$FILE"; then
  echo "--> test data dir exists"
else
  ln -s ../../test_data .
fi

# generate C executables
make 2d.out 3d.out

CAPI=output.capi
CPPAPI=output.cppapi

#
# test 2D BPP
#
FILE=./test_data/lena512.float 
./2d.out $FILE 512 512 1 2.5
./bin/compressor_2d --dims 512 512 --out_bitstream output.stream --bpp 2.5 $FILE
./bin/decompressor_2d -o $CPPAPI output.stream

if diff $CAPI $CPPAPI; then
  echo "--> C and C++ utilities produce the same result on test data $FILE"
else
  echo "--> C and C++ utilities produce different results on test data $FILE"
  exit 1
fi
rm -f $CAPI $CPPAPI

#
# test 2D PSNR
#
FILE=./test_data/lena512.float 
./2d.out $FILE 512 512 2 95.0
./bin/compressor_2d --dims 512 512 --out_bitstream output.stream --psnr 95.0 $FILE
./bin/decompressor_2d -o $CPPAPI output.stream

if diff $CAPI $CPPAPI; then
  echo "--> C and C++ utilities produce the same result on test data $FILE"
else
  echo "--> C and C++ utilities produce different results on test data $FILE"
  exit 1
fi
rm -f $CAPI $CPPAPI

#
# test 2D PWE
#
FILE=./test_data/lena512.float 
./2d.out $FILE 512 512 3 0.5
./bin/compressor_2d --dims 512 512 --out_bitstream output.stream --pwe 0.5 $FILE
./bin/decompressor_2d -o $CPPAPI output.stream

if diff $CAPI $CPPAPI; then
  echo "--> C and C++ utilities produce the same result on test data $FILE"
else
  echo "--> C and C++ utilities produce different results on test data $FILE"
  exit 1
fi
rm -f $CAPI $CPPAPI


#
# test 3D BPP
#
FILE=./test_data/density_128x128x256.d64
./3d.out $FILE 128 128 256 1 2.5 -d
./bin/compressor_3d --dims 128 128 256 -d --out_bitstream output.stream --bpp 2.5 $FILE
./bin/decompressor_3d -o $CPPAPI -d output.stream

if diff $CAPI $CPPAPI; then
  echo "--> C and C++ utilities produce the same result on test data $FILE"
else
  echo "--> C and C++ utilities produce different results on test data $FILE"
  exit 1
fi
rm -f $CAPI $CPPAPI

#
# test 3D PSNR
#
FILE=./test_data/density_128x128x256.d64
./3d.out $FILE 128 128 256 2 102.5 -d
./bin/compressor_3d --dims 128 128 256 -d --out_bitstream output.stream --psnr 102.5 $FILE
./bin/decompressor_3d -o $CPPAPI -d output.stream

if diff $CAPI $CPPAPI; then
  echo "--> C and C++ utilities produce the same result on test data $FILE"
else
  echo "--> C and C++ utilities produce different results on test data $FILE"
  exit 1
fi
rm -f $CAPI $CPPAPI

#
# test 3D PWE
#
FILE=./test_data/density_128x128x256.d64
./3d.out $FILE 128 128 256 3 2.5e-5 -d
./bin/compressor_3d --dims 128 128 256 -d --out_bitstream output.stream --pwe 2.5e-5 $FILE
./bin/decompressor_3d -o $CPPAPI -d output.stream

if diff $CAPI $CPPAPI; then
  echo "--> C and C++ utilities produce the same result on test data $FILE"
else
  echo "--> C and C++ utilities produce different results on test data $FILE"
  exit 1
fi
rm -f $CAPI $CPPAPI
