/*
 * This header provides C API for SPERR.
 * This API is supposed to be used in non-C++ (e.g., C, Fortran, Python) projects.
 */

#ifndef SPERR_C_API_H
#define SPERR_C_API_H

#ifndef USE_VANILLA_CONFIG
#include "SperrConfig.h"
#endif

#include <stddef.h> /* for size_t */
#include <stdint.h> /* for fixed-width integers */

#ifdef __cplusplus
namespace C_API {
extern "C" {
#endif

/*
 * The memory management is a little tricy. The following requirement applies to the output
 * buffer `dst` of all the C API functions.
 *
 * The output is stored in `dst`, which is a pointer pointing to another pointer
 * held by the caller.  The other pointer should be NULL; otherwise this function will fail!
 * Upon success, `dst` will contain a buffer of length `dst_len` in case of compression,
 * and (dimx x dimy x dimz) of floats (or doubles) in case of decompression.
 * The caller of this function is responsible of free'ing `dst` using free().
 *
 */

/*
 * Compress a a 2D slice targetting different quality controls (modes):
 *   mode == 1 --> fixed bit-per-pixel (BPP)
 *   mode == 2 --> fixed peak signal-to-noise ratio (PSNR)
 *   mode == 3 --> fixed point-wise error (PWE)
 *
 * Return value meanings:
 * 0: success
 * 1: `dst` is not pointing to a NULL pointer!
 * 2: `mode` isn't valid
 * 3: `is_float` value not supported
 *-1: other error
 */
int sperr_comp_2d(
    const void* src,  /* Input: buffer that contains a 2D slice */
    int is_float,     /* Input: input buffer type: 1 == float, 0 == double */
    size_t dimx,      /* Input: X (fastest-varying) dimension */
    size_t dimy,      /* Input: Y (slowest-varying) dimension */
    int mode,         /* Input: compression mode to use */
    double quality,   /* Input: target quality */
    void** dst,       /* Output: buffer for the output bitstream, allocated by this function */
    size_t* dst_len); /* Output: length of `dst` in byte */

/*
 * Decompress a 2D SPERR-compressed buffer.
 *
 * Return value meanings:
 * 0: success
 * 1: `dst` not pointing to a NULL pointer!
 * 2: `output_float` value not supported
 * -1: other error
 */
int sperr_decomp_2d(
    const void* src,  /* Input: buffer that contains a compressed bitstream */
    size_t src_len,   /* Input: length of the input bitstream in byte */
    int output_float, /* Input: output data type: 1 == float, 0 == double */
    size_t* dimx,     /* Output: X (fast-varying) dimension */
    size_t* dimy,     /* Output: Y (slowest-varying) dimension */
    void** dst);      /* Output: buffer for the output 2D slice, allocated by this function */

/*
 * Compress a a 3D volume targetting different quality controls (modes):
 *   mode == 1 --> fixed bit-per-pixel (BPP)
 *   mode == 2 --> fixed peak signal-to-noise ratio (PSNR)
 *   mode == 3 --> fixed point-wise error (PWE)
 *
 * Return value meanings:
 * 0: success
 * 1: `dst` is not pointing to a NULL pointer!
 * 2: `mode` or `quality` isn't valid
 * 3: `is_float` value not supported
 *-1: other error
 */
int sperr_comp_3d(
    const void* src,  /* Input: buffer that contains a 3D volume */
    int is_float,     /* Input: input buffer type: 1 == float, 0 = double */
    size_t dimx,      /* Input: X (fastest-varying) dimension */
    size_t dimy,      /* Input: Y dimension */
    size_t dimz,      /* Input: Z (slowest-varying) dimension */
    size_t chunk_x,   /* Input: preferred chunk dimension in X */
    size_t chunk_y,   /* Input: preferred chunk dimension in Y */
    size_t chunk_z,   /* Input: preferred chunk dimension in Z */
    int mode,         /* Input: compression mode to use */
    double quality,   /* Input: target quality */
    size_t nthreads,  /* Input: number of OpenMP threads to use. 0 means using all threads. */
    void** dst,       /* Output: buffer for the output bitstream, allocated by this function */
    size_t* dst_len); /* Output: length of `dst` in byte */

/*
 * Decompress a 3D SPERR-compressed buffer.
 *
 * Return value meanings:
 * 0: success
 * 1: `dst` not pointing to a NULL pointer!
 * 2: `output_float` value not supported
 * -1: other error
 */
int sperr_decomp_3d(
    const void* src,  /* Input: buffer that contains a compressed bitstream */
    size_t src_len,   /* Input: length of the input bitstream in byte */
    int output_float, /* Input: output data type: 1 == float, 0 == double */
    size_t nthreads,  /* Input: number of OMP threads to use. 0 means using all threads. */
    size_t* dimx,     /* Output: X (fast-varying) dimension */
    size_t* dimy,     /* Output: Y dimension */
    size_t* dimz,     /* Output: Z (slowest-varying) dimension */
    void** dst);      /* Output: buffer for the output 3D slice, allocated by this function */

#ifdef __cplusplus
} /* end of extern "C" */
} /* end of namespace C_API */
#endif

#endif
