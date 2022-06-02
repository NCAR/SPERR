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
make size 

CAPI=output.capi
CPPAPI=output.cppapi

#
# test 2D case
#
FILE=./test_data/lena512.float 
./2d_size.out $FILE 512 512 2.5
./bin/compressor_2d --dims 512 512 -o output.stream --bpp 2.5 $FILE
./bin/decompressor_2d -o $CPPAPI output.stream

if diff $CAPI $CPPAPI; then
  echo "--> C and C++ utilities produce the same result on test data $FILE"
else
  echo "--> C and C++ utilities produce different results on test data $FILE"
fi
rm -f $CAPI $CPPAPI


#
# test: 3D with outlier correction
#
FILE=./test_data/vorticity.128_128_41
./3d_size.out $FILE 128 128 41 2.5
./bin/compressor_3d --dims 128 128 41 -o output.stream --bpp 2.5 $FILE
./bin/decompressor_3d -o $CPPAPI output.stream

if diff $CAPI $CPPAPI; then
  echo "--> C and C++ utilities produce the same result on test data $FILE"
else
  echo "--> C and C++ utilities produce different results on test data $FILE"
fi
rm -f $CAPI $CPPAPI
