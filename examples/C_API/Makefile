
UNAME = $(shell uname)
ifeq (${UNAME}, Darwin)
  CXX = g++-11
  CC = gcc-11
  SPERR_INCDIR=/Users/shaomeng/Git/SPERR/install/include
else
  CXX = g++
  CC = gcc
  SPERR_INCDIR=/glade/u/home/shaomeng/Git/SPERR/install/include
endif

FLAGS = -O3 -march=native
COMP_LIBS = libSPERR.a

all: test.out

test.out: test.c ${COMP_LIBS}
	${CC} ${FLAGS} -o $@ $^ -I${SPERR_INCDIR} -fopenmp

clean:
	rm -f test.out
