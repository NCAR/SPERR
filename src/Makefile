CC=gcc
CXX=g++
CXXFLAGS=-std=c++14 -Wall -g -O2 -lm -Wno-sign-compare

ARCH=$(shell uname)
ifeq ($(ARCH), Darwin)
  CC=clang
  CXX=clang++
  CXXFLAGS=-std=c++14 -Wall -g 
endif

all:

SPECK2D.o: SPECK2D.cpp speck_helper.o
	$(CXX) $(CXXFLAGS) $< -c -o $@

CDF97.o: CDF97.cpp speck_helper.o
	$(CXX) $(CXXFLAGS) $< -c -o $@

speck_helper.o: speck_helper.cpp speck_helper.h
	$(CXX) $(CXXFLAGS) $< -c -o $@

a.out: main.cpp speck_helper.o
	$(CXX) $(CXXFLAGS) $^ -o $@


clean:
	rm -rf *.out* *.o *.tmp
