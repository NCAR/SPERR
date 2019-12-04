CC=gcc
CXX=g++
CXXFLAGS=-O2 -g -Wall

ARCH=$(shell uname)
ifeq ($(ARCH), Darwin)
  CC=clang
  CXX=clang++
endif

all:

float2nc.out: float2nc.c
	$(CC) $< -o $@ -lnetcdf


clean:
	rm bin2ascii dec2hex calcerr input2bin readrandom calcerr maxdiff cropcube zeroout cropanysize
