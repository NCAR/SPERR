#include "SPERR_C_API.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

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
  int rtn = fread(buf, 1, len, f);
  assert(rtn == len);
  fclose(f);
  *dst = buf;

  return len;
}

int main(int argc, char** argv)
{
  if (argc < 6) {
    printf("Usage: ./a.out filename dimx dimy mode quality [-d]\n");
    printf("  Note: -d is optional to indicate that the input is in double format\n");
    return 1;
  }

  const char* filename = argv[1];
  const size_t dimx = (size_t)atol(argv[2]);
  const size_t dimy = (size_t)atol(argv[3]);
  const int mode = (int)atoi(argv[4]);
  const double quality = atof(argv[5]);
  int is_float = 1;
  if (argc == 7)
    is_float = 0;

  /* Read in a file and put its content in `inbuf` */
  void* inbuf = NULL; /* Will be free'd later */
  size_t inlen = read_file(filename, &inbuf);
  if (inlen == 0)
    return 1;
  if (is_float && inlen != sizeof(float) * dimx * dimy) {
    free(inbuf);
    return 1;
  }
  if (!is_float && inlen != sizeof(double) * dimx * dimy) {
    free(inbuf);
    return 1;
  }

  /* Compress `inbuf` and put the compressed bitstream in `bitstream` */
  void* bitstream = NULL; /* Will be free'd later */
  size_t stream_len = 0;
  int rtn = sperr_comp_2d(inbuf, is_float, dimx, dimy, mode, quality, &bitstream, &stream_len);
  if (rtn != 0) {
    printf("Compression error with code %d\n", rtn);
    return rtn;
  }

  /* Write the compressed bitstream to disk */
  FILE* f = fopen("./output.stream", "w");
  assert(f != NULL);
  fwrite(bitstream, 1, stream_len, f);
  fclose(f);
  f = NULL;

  /* Decompress `bitstream` and put the decompressed slice in `outbuf` */
  void* outbuf = NULL; /* Will be free'd later */
  size_t out_dimx = 0;
  size_t out_dimy = 0;
  rtn = sperr_decomp_2d(bitstream, stream_len, is_float, &out_dimx, &out_dimy, &outbuf);
  if (rtn != 0) {
    printf("Decompression error with code %d\n", rtn);
    return 1;
  }
  if (out_dimx != dimx || out_dimy != dimy) {
    printf("Decompression dimensions wrong!\n");
    return 1;
  }

  /* Write the decompressed buffer to disk */
  f = fopen("./output.data", "w");
  assert(f != NULL);
  if (is_float)
    fwrite(outbuf, sizeof(float), out_dimx * out_dimy, f);
  else
    fwrite(outbuf, sizeof(double), out_dimx * out_dimy, f);
  fclose(f);

  /* Clean up */
  free(outbuf);
  free(bitstream);
  free(inbuf);
}
