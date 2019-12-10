CC=gcc
CXX=g++
CFLAGS=-O2 -g -Wall -lm

ARCH=$(shell uname)
ifeq ($(ARCH), Darwin)
  CC=clang
  CXX=clang++
  CFLAGS=-O2 -g -Wall 
endif

QCC_INSTALL_DIR=../QccPack-0.61-1/install

all:

float2nc.out: float2nc.c
	$(CC) $< -o $@ -lnetcdf

crop2d.out: crop2d.c
	$(CC) $< -o $@

evaluate2d.out: evaluate2d.c helper.o
	$(CC) $(CFLAGS) -I$(QCC_INSTALL_DIR)/include $^ $(QCC_INSTALL_DIR)/lib/libQccPack.a -o $@

helper.o: helper.c helper.h
	$(CC) $(CFLAGS) $< -c -o $@


clean:
	rm -rf *.out* *.o *.tmp
