/*
 * This header provides C API for SPERR.
 * This API is supposed to be used in C and Fortran projects.
 *
 * As of 4/13/2022, this API is only implemented for the fixed-error mode of SPERR.
 */

#ifndef SPERR_C_API_H
#define SPERR_C_API_H

#ifdef QZ_TERM
/*
 * Compress a buffer that contains a 2D slice in 64-bit float type.
 *
 * The output buffer `dst` should have been allocated by the caller with a length `dst_len`.
 * If the compressed bitstream is shorter than `dst_len`, then the bitstream will be
 * placed in `dst` with its actual length placed in `useful_dst_len`.
 * Otherwise, this function will fail with a return value of 1.
 * When in doubt, allocate the output buffer with the same length of input.
 *
 * Return value meanings:
 * 0: success
 * 1: `dst` buffer too short
 * 2: `qlev` too big for this input
 * 3: `is_float` value not supported
 * -1: other errors
 */
extern "C" int sperr_qzcomp_2d(
    const void* src,         /* Input: buffer that contains a 2D slice */
    int32_t is_float,        /* Input: input buffer type: 1 == float, 0 == double */
    size_t dimx,             /* Input: X (fastest-varying) dimension */
    size_t dimy,             /* Input: Y (slowest-varying) dimension */
    int32_t qlev,            /* Input: q (quantization) level for SPERR */
    double tol,              /* Input: tolerance for SPERR */
    void* dst,               /* Output: pre-allocated buffer for output bitstream */
    size_t dst_len,          /* Output: allocated length of `dst` in byte */
    size_t* useful_dst_len); /* Output: length of valid bitstream in `dst` */


/* finish fixed-error mode */
#else
/* fixed-size mode */

#endif
#endif
