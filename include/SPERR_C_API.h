/*
 * This header provides C API for SPERR.
 * This API is supposed to be used in non-C++ (e.g., C, Fortran, Python) projects.
 */

#ifndef SPERR_C_API_H
#define SPERR_C_API_H

#include "SperrConfig.h"

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

#ifdef QZ_TERM

/*
 * Compress a buffer that contains a 2D slice in fixed-error mode.
 *
 * Return value meanings:
 * 0: success
 * 1: `dst` is not pointing to a NULL pointer!
 * 2: `qlev` too big for this input
 * 3: `is_float` value not supported
 * -1: other errors
 */
int sperr_qzcomp_2d(
    const void* src,  /* Input: buffer that contains a 2D slice */
    int32_t is_float, /* Input: input buffer type: 1 == float, 0 == double */
    size_t dimx,      /* Input: X (fastest-varying) dimension */
    size_t dimy,      /* Input: Y (slowest-varying) dimension */
    int32_t qlev,     /* Input: q (quantization) level */
    double tol,       /* Input: absolute error tolerance */
    void** dst,       /* Output: buffer for the output bitstream, allocated by this function */
    size_t* dst_len); /* Output: length of `dst` in byte */

/*
 * Compress a buffer that contains a 3D volume in fixed-error mode.
 *
 * Return value meanings:
 * 0: success
 * 1: `dst` is not pointing to a NULL pointer!
 * 2: `qlev` too big for this input
 * 3: `is_float` value not supported
 * -1: other errors
 */
int sperr_qzcomp_3d(
    const void* src,  /* Input: buffer that contains a 3D volume */
    int32_t is_float, /* Input: input buffer type: 1 == float, 0 = double */
    size_t dimx,      /* Input: X (fastest-varying) dimension */
    size_t dimy,      /* Input: Y dimension */
    size_t dimz,      /* Input: Z (slowest-varying) dimension */
    int32_t qlev,     /* Input: q (quantization) level */
    double tol,       /* Input: absolute error tolerance */
    int32_t nthreads, /* Input: number of OMP threads to use */
    void** dst,       /* Output: buffer for the output bitstream, allocated by this function */
    size_t* dst_len); /* Output: length of `dst` in byte */

#else

/* fixed-size mode functions */

/*
 * Compress a buffer that contains a 2D slice in fixed-size mode.
 *
 * Return value meanings:
 * 0: success
 * 1: `dst` is not pointing to a NULL pointer!
 * 2: `bpp` is not valid (e.g., too small)
 * 3: `is_float` value not supported
 * -1: other errors
 */
int sperr_sizecomp_2d(
    const void* src,  /* Input: buffer that contains a 2D slice */
    int32_t is_float, /* Input: input buffer type: 1 == float, 0 == double */
    size_t dimx,      /* Input: X (fastest-varying) dimension */
    size_t dimy,      /* Input: Y (slowest-varying) dimension */
    double bpp,       /* Input: target bit-per-pixel */
    void** dst,       /* Output: buffer for the output bitstream, allocated by this function */
    size_t* dst_len); /* Output: length of `dst` in byte */

/*
 * Compress a buffer that contains a 3D volume in fixed-size mode.
 *
 * Return value meanings:
 * 0: success
 * 1: `dst` is not pointing to a NULL pointer!
 * 2: `bpp` is not valid (e.g., too small)
 * 3: `is_float` value not supported
 * -1: other errors
 */
int sperr_sizecomp_3d(
    const void* src,  /* Input: buffer that contains a 3D volume */
    int32_t is_float, /* Input: input buffer type: 1 == float, 0 == double */
    size_t dimx,      /* Input: X (fastest-varying) dimension */
    size_t dimy,      /* Input: Y dimension */
    size_t dimz,      /* Input: Z (slowest-varying) dimension */
    double bpp,       /* Input: target bit-per-pixel */
    int32_t nthreads, /* Input: number of OMP threads to use */
    void** dst,       /* Output: buffer for the output bitstream, allocated by this function */
    size_t* dst_len); /* Output: length of `dst` in byte */

#endif

/* Functions in both fixed-error and fixed-size mode. */

/*
 * Decompress a 2D SPERR-compressed buffer in either fixed-error or fixed-size mode.
 *
 * Return value meanings:
 * 0: success
 * 1: `dst` not pointing to a NULL pointer!
 * 2: `output_float` value not supported
 * -1: other errors
 */
int sperr_decomp_2d(
    const void* src,      /* Input: buffer that contains a compressed bitstream */
    size_t src_len,       /* Input: length of the input bitstream in byte */
    int32_t output_float, /* Input: output data type: 1 == float, 0 == double */
    size_t* dimx,         /* Output: X (fast-varying) dimension */
    size_t* dimy,         /* Output: Y (slowest-varying) dimension */
    void** dst);          /* Output: buffer for the output 2D slice, allocated by this function */

/*
 * Decompress a 3D SPERR-compressed buffer in fixed-error and fixed-size mode.
 *
 * Return value meanings:
 * 0: success
 * 1: `dst` not pointing to a NULL pointer!
 * 2: `output_float` value not supported
 * -1: other errors
 */
int sperr_decomp_3d(
    const void* src,      /* Input: buffer that contains a compressed bitstream */
    size_t src_len,       /* Input: length of the input bitstream in byte */
    int32_t output_float, /* Input: output data type: 1 == float, 0 == double */
    int32_t nthreads,     /* Input: number of OMP threads to use */
    size_t* dimx,         /* Output: X (fast-varying) dimension */
    size_t* dimy,         /* Output: Y dimension */
    size_t* dimz,         /* Output: Z (slowest-varying) dimension */
    void** dst);          /* Output: buffer for the output 3D slice, allocated by this function */

/*
 * Given a SPERR bitstream, parse the header and retrieve various types of information.
 *
 * Return value meanings:
 * 0: success
 * 1: parsing error occured
 */
void sperr_parse_header(
    const void* ptr,        /* Input: the bitstream to parse */
    int32_t* version_major, /* Output: major version number */
    int32_t* zstd_applied,  /* Output: if ZSTD applied (0 == no ZSTD; 1 == ZSTD applied) */
    int32_t* is_qz_term,    /* Output: compression mode (0 == fixed-size; 1 == fixed-error) */
    int32_t* is_3d,         /* Output: 3D volume or 2D slice (0 == 2D, 1 == 3D) */
    uint32_t* dim_x,        /* Output: X dimension */
    uint32_t* dim_y,        /* Output: Y dimension */
    uint32_t* dim_z);       /* Output: Z dimension */

#ifdef __cplusplus
} /* end of extern "C" */
} /* end of namespace C_API */
#endif

#endif
