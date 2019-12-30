CC=gcc
CXX=g++
CFLAGS=-O2 -g -Wall -lm -Wno-unused-variable
CXXFLAGS=-std=c++11 -O2 -g -Wall -Wno-unused-variable -Wno-unused-private-field -lm

ARCH=$(shell uname)
ifeq ($(ARCH), Darwin)
  CC=clang
  CXX=clang++
  CFLAGS=-O2 -g -Wall -Wno-unused-variable
  CXXFLAGS=-std=c++11 -O2 -g -Wall -Wno-unused-private-field -Wno-unused-variable
endif

QCC_INSTALL_DIR=../QccPack-0.61-1/install
SPECK_DIR=../src

all:

float2nc.out: float2nc.c
	$(CC) $< -o $@ -lnetcdf

crop2d.out: crop2d.c
	$(CC) $< -o $@

evaluate2d_qcc.out: evaluate2d_qcc.c helper.o
	$(CC) $(CFLAGS) -I$(QCC_INSTALL_DIR)/include $^ $(QCC_INSTALL_DIR)/lib/libQccPack.a -o $@

helper.o: helper.c helper.h
	$(CC) $(CFLAGS) $< -c -o $@

test.out: test.c
	$(CC) $(CFLAGS) -I$(QCC_INSTALL_DIR)/include $^ $(QCC_INSTALL_DIR)/lib/libQccPack.a -o $@

test_sam_dwt2d.out: test_sam_dwt2d.cpp helper.o
	$(CXX) $(CXXFLAGS) -I$(SPECK_DIR) $^ $(SPECK_DIR)/CDF97.o -o $@

test_qcc_dwt2d.out: test_qcc_dwt2d.c helper.o
	$(CC) $(CFLAGS) -I$(QCC_INSTALL_DIR)/include $^ $(QCC_INSTALL_DIR)/lib/libQccPack.a -o $@

compare_raw.out: compare_raw.c helper.o
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf *.out* *.o *.tmp
