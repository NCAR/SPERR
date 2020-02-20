CC=gcc
CXX=g++
CXXFLAGS=-std=c++14 -O2 -Wall -g -lm

ARCH=$(shell uname)
ifeq ($(ARCH), Darwin)
  CC=gcc-9
  CXX=g++-9
  CXXFLAGS=-std=c++14 -O2 -Wall -g 
endif

all:

SPECK2D.o: SPECK2D.cpp speck_helper.o
	$(CXX) $(CXXFLAGS) $< -c -o $@

CDF97.o: CDF97.cpp speck_helper.o
	$(CXX) $(CXXFLAGS) $< -c -o $@

speck_helper.o: speck_helper.cpp speck_helper.h
	$(CXX) $(CXXFLAGS) $< -c -o $@


clean:
	rm -rf *.out* *.o *.tmp
