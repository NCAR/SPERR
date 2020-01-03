CC=gcc
CXX=g++
CXXFLAGS=-std=c++14 -O2 -Wall -g -lm

ARCH=$(shell uname)
ifeq ($(ARCH), Darwin)
  CC=clang
  CXX=clang++
  CXXFLAGS=-std=c++14 -O2 -Wall -g 
endif

all:

SPECK.o: SPECK.cpp SPECK.h
	$(CXX) $(CXXFLAGS) $< -c -o $@

CDF97.o: CDF97.cpp CDF97.h
	$(CXX) $(CXXFLAGS) $< -c -o $@


clean:
	rm -rf *.out* *.o *.tmp
