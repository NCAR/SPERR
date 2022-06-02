
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

FLAGS = -O3

qz: 2d_qz.out 3d_qz.out
size: 2d_size.out 3d_size.out

2d_qz.out: 2d_qz.c
	${CC} ${FLAGS} -o $@ $^ -I${SPERR_INCDIR} -L${SPERR_LIBDIR} -Wl,-rpath,${SPERR_LIBDIR} -lSPERR

3d_qz.out: 3d_qz.c
	${CC} ${FLAGS} -o $@ $^ -I${SPERR_INCDIR} -L${SPERR_LIBDIR} -Wl,-rpath,${SPERR_LIBDIR} -lSPERR

2d_size.out: 2d_size.c
	${CC} ${FLAGS} -o $@ $^ -I${SPERR_INCDIR} -L${SPERR_LIBDIR} -Wl,-rpath,${SPERR_LIBDIR} -lSPERR

3d_size.out: 3d_size.c
	${CC} ${FLAGS} -o $@ $^ -I${SPERR_INCDIR} -L${SPERR_LIBDIR} -Wl,-rpath,${SPERR_LIBDIR} -lSPERR


clean:
	rm -f 2d_qz.out 3d_qz.out 2d_size.out
