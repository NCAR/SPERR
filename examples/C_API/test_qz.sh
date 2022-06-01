#!/bin/bash

# create a few symbolic links
FILE=bin
if test -e "$FILE"; then
  echo "C++ bin dir exists"
else
  ln -s ../../install/bin .
fi

FILE=test_data
if test -e "$FILE"; then
  echo "test data dir exists"
else
  ln -s ../../test_data .
fi

# generate C executables
make qz

CAPI=output.capi
CPPAPI=output.cppapi

#
# test: 2D with no outlier correction
#
FILE=./test_data/lena512.float 
./2d_qz.out $FILE 512 512 -1 1
./bin/compressor_2d --dims 512 512 -o output.stream -q -1 $FILE
./bin/decompressor_2d -o $CPPAPI output.stream

if diff $CAPI $CPPAPI; then
  echo "C and C++ utilities produce the same result on test data $FILE"
else
  echo "C and C++ utilities produce different results on test data $FILE"
fi
rm -f $CAPI $CPPAPI

#
# test: 2D with outlier correction
#
FILE=./test_data/999x999.float 
./2d_qz.out $FILE 999 999 -1 0.8
./bin/compressor_2d --dims 999 999 -o output.stream -q -1 -t 0.8 $FILE
./bin/decompressor_2d -o $CPPAPI output.stream

if diff $CAPI $CPPAPI; then
  echo "C and C++ utilities produce the same result on test data $FILE"
else
  echo "C and C++ utilities produce different results on test data $FILE"
fi
rm -f $CAPI $CPPAPI

#
# test: 3D with no outlier correction
#
FILE=./test_data/wmag128.float 
./3d_qz.out $FILE 128 128 128 -1 2
./bin/compressor_3d --dims 128 128 128 -o output.stream -q -1 $FILE
./bin/decompressor_3d -o $CPPAPI output.stream

if diff $CAPI $CPPAPI; then
  echo "C and C++ utilities produce the same result on test data $FILE"
else
  echo "C and C++ utilities produce different results on test data $FILE"
fi
rm -f $CAPI $CPPAPI

#
# test: 3D with outlier correction
#
FILE=./test_data/vorticity.128_128_41
./3d_qz.out $FILE 128 128 41 -20 1.5e-6
./bin/compressor_3d --dims 128 128 41 -o output.stream -q -20 -t 1.5e-6 $FILE
./bin/decompressor_3d -o $CPPAPI output.stream

if diff $CAPI $CPPAPI; then
  echo "C and C++ utilities produce the same result on test data $FILE"
else
  echo "C and C++ utilities produce different results on test data $FILE"
fi
rm -f $CAPI $CPPAPI
