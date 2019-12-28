CC=gcc
CXX=g++
CFLAGS=-std=c++11 -g -Wall -lm

ARCH=$(shell uname)
ifeq ($(ARCH), Darwin)
  CC=clang
  CXX=clang++
  CFLAGS=-std=c++11 -g -Wall -Wno-unused-private-field
endif

all:

main.out: main.cpp CDF97.o
	$(CXX) $(CFLAGS) $^ -o $@

CDF97.o: CDF97.cpp CDF97.h
	$(CXX) $(CFLAGS) $< -c -o $@


clean:
	rm -rf *.out* *.o *.tmp
