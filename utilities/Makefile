CC=gcc
CXX=g++
CXXFLAGS=-O2 -g -Wall

ARCH=$(shell uname)
ifeq ($(ARCH), Darwin)
  CC=clang
  CXX=clang++
endif

QCC_INSTALL_DIR=../QccPack-0.61-1/install

all:

float2nc.out: float2nc.c
	$(CC) $< -o $@ -lnetcdf

crop2d.out: crop2d.c
	$(CC) $< -o $@

evaluate2d.out: evaluate2d.c
	$(CC) $< -I$(QCC_INSTALL_DIR)/include $(QCC_INSTALL_DIR)/lib/libQccPack.a -o $@


clean:
	rm -f *.out *.o
