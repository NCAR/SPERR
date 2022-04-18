#include "SPERR_C_API.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

/* 
 * Given a file name, this function reads in its content and allocates a buffer `dst` to store it.
 * Upon success, it returns the size of the allocated buffer `dst`. Otherwise, it returns 0.
 * The caller is responsible of free'ing buffer `dst` using free().
 */
size_t read_file(const char* filename, void** dst)
{
  FILE* f = fopen(filename, "r");
  if (!f)
    return 0;
  fseek(f, 0, SEEK_END);
  const size_t len = ftell(f);
  fseek(f, 0, SEEK_SET);
  uint8_t* buf = malloc(len);
  fread(buf, 1, len, f);
  fclose(f);
  *dst = buf;

  return len;
}


int main(int argc, char** argv)
{
  if (argc < 6) {
    printf("Usage: ./a.out filename dimx, dimy, qz_lev, tolerance [-d]\n");
    printf("  Note: -d is optional to indicate that the input is in double format\n");
    return 1;
  }

  const char* filename = argv[1];
  const size_t dimx = (size_t)atol(argv[2]);
  const size_t dimy = (size_t)atol(argv[3]);
  const int32_t q = (int32_t)atoi(argv[4]);
  const double tol = atof(argv[5]);
  int is_float = 1;
  if (argc == 7)
    is_float = 0;

  /* Read in a file and put its content in `inbuf` */
  void* inbuf = NULL;
  size_t inlen = read_file(filename, &inbuf);
  if (inlen == 0)
    return 1;

  /* Compress `inbuf` and put the compressed bitstream in `bitstream` */
  void* bitstream = NULL;
  size_t stream_len = 0;
  /* int rtn = sperr_qzcomp_2d(inbuf, is_float, dimx, dimy, q, tol, &bitstream, &stream_len); */
  int rtn = boring_f(tol, q);
  printf("boring_f = %d\n", rtn);
  if (rtn != 0) {
    printf("Compression error with code %d\n", rtn);
    return 1;
  }

  /* Write the compressed bitstream to disk */
  FILE* f = fopen("./out.stream", "w");
  assert(f != NULL);
  fwrite(bitstream, 1, stream_len, f);
  fclose(f);
}
