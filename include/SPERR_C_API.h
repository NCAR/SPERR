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
 * Compress a buffer that contains a 2D slice in float or double type.
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
    size_t dst_len,          /* Intput: allocated buffer length of `dst` in byte */
    void* dst,               /* Output: pre-allocated buffer for output bitstream */
    size_t* useful_dst_len); /* Output: length of valid bitstream in `dst` */

/*
 * Decompress a 2D SPERR-compressed buffer.
 *
 * The memory management is a little tricy; please read carefully!
 *
 * The output 2D slice is stored in `dst`, which is a pointer pointing to another pointer.
 * The other pointer should be NULL; otherwise this function will fail!
 * Upon success, `dst` will contain (`dimx` x `dimy`) doubles or floats as requested.
 * The caller of this function is responsible of free'ing `dst` using free().
 *
 *
 * Return value meanings:
 * 0: success
 * 1: `dst` not pointing to a NULL pointer!
 * 2: `output_float` value not supported
 * -1: other errors
 */
extern "C" int sperr_qzdecomp_2d(
    const void* src,        /* Input: buffer that contains a compressed bitstream */
    size_t src_len,         /* Input: length of the input bitstream in byte */
    int32_t output_float,   /* Input: output data type: 1 == float, 0 == double */
    size_t* dimx,           /* Output: X (fast-varying) dimension */
    size_t* dimy,           /* Output: Y (slowest-varying) dimension */
    void** dst);            /* Output: buffer for the output 2D slice, allocated by this function */


/* finish fixed-error mode */
#else
/* fixed-size mode */

#endif
#endif
