
UNAME = $(shell uname)
ifeq (${UNAME}, Darwin)
  CXX = g++-11
  CC = gcc-11
  SPERR_INCDIR=/Users/shaomeng/Git/SPERR/install/include
  SPERR_LIBDIR=/Users/shaomeng/Git/SPERR/install/lib
else
  CXX = g++
  CC = gcc
  SPERR_INCDIR=/home/shaomeng/Git/SPERR/install/include
  SPERR_LIBDIR=/home/shaomeng/Git/SPERR/install/lib
endif

FLAGS = -O3 -march=native
COMP_LIBS = libSPERR.a

all: 2d_qz.out 3d_qz.out

2d_qz.out: 2d_qz.c
	${CC} ${FLAGS} -o $@ $^ -I${SPERR_INCDIR} -L${SPERR_LIBDIR} -Wl,-rpath,${SPERR_LIBDIR} -lSPERR -fopenmp

3d_qz.out: 3d_qz.c
	${CC} ${FLAGS} -o $@ $^ -I${SPERR_INCDIR} -L${SPERR_LIBDIR} -Wl,-rpath,${SPERR_LIBDIR} -lSPERR -fopenmp

clean:
	rm -f 2d_qz.out 3d_qz.out
