CC=gcc
CXX=g++
CFLAGS=-O2 -g -Wall -lm -Wno-unused-variable
CXXFLAGS=-std=c++14 -O2 -g -Wall -Wno-unused-variable -Wno-unused-private-field -lm

ARCH=$(shell uname)
ifeq ($(ARCH), Darwin)
  CC=clang
  CXX=clang++
  CFLAGS=-O2 -g -Wall -Wno-unused-variable
  CXXFLAGS=-std=c++14 -O2 -g -Wall -Wno-unused-private-field -Wno-unused-variable
endif

QCC_INSTALL_DIR=../QccPack-0.61-1/install
SPECK_DIR=../src

all:

float2nc.out: float2nc.c
	$(CC) $< -o $@ -lnetcdf

crop2d.out: crop2d.c
	$(CC) $< -o $@

test_qcc_speck2d.out: test_qcc_speck2d.c helper.o
	$(CC) $(CFLAGS) -I$(QCC_INSTALL_DIR)/include $^ $(QCC_INSTALL_DIR)/lib/libQccPack.a -o $@

test_sam_speck2d.out: test_sam_speck2d.cpp helper.o $(SPECK_DIR)/SPECK2D.o  $(SPECK_DIR)/speck_helper.o 
	$(CXX) $(CXXFLAGS) -I$(SPECK_DIR) $^ -o $@

helper.o: helper.c helper.h
	$(CC) $(CFLAGS) $< -c -o $@

main.out: main.c
	$(CC) $(CFLAGS) -I$(QCC_INSTALL_DIR)/include $^ $(QCC_INSTALL_DIR)/lib/libQccPack.a -o $@

test_sam_dwt2d.out: test_sam_dwt2d.cpp helper.o $(SPECK_DIR)/CDF97.o $(SPECK_DIR)/speck_helper.o 
	$(CXX) $(CXXFLAGS) -I$(SPECK_DIR) $^ -o $@

test_qcc_dwt2d.out: test_qcc_dwt2d.c helper.o
	$(CC) $(CFLAGS) -I$(QCC_INSTALL_DIR)/include $^ $(QCC_INSTALL_DIR)/lib/libQccPack.a -o $@

compare_raw.out: compare_raw.c helper.o
	$(CC) $(CFLAGS) $^ -o $@

expt.out: expt.cpp
	$(CXX) $(CXXFLAGS) -I$(SPECK_DIR) $(SPECK_DIR)/SPECK.o $(SPECK_DIR)/speck_helper.o $^ -o $@

clean:
	rm -rf *.out* *.o *.tmp
