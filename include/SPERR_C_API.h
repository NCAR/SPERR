/*
 * This header provides C API for SPERR.
 * This API is supposed to be used in C and Fortran projects.
 *
 * As of 4/13/2022, this API is only implemented for the fixed-error mode of SPERR.
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

#ifdef QZ_TERM

/*
 * Compress a buffer that contains a 2D slice in fixed-error mode.
 *
 * The memory management is a little tricy; please read carefully!
 *
 * The output bitstream is stored in `dst`, which is a pointer pointing to another pointer
 * held by the caller.  The other pointer should be NULL; otherwise this function will fail!
 * Upon success, `dst` will contain a bitstream of length `dst_len`.
 * The caller of this function is responsible of free'ing `dst` using free().
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
 * Decompress a 2D SPERR-compressed buffer in fixed-error mode.
 *
 * The memory management is a little tricy; please read carefully!
 *
 * The output 2D slice is stored in `dst`, which is a pointer pointing to another pointer
 * held by the caller.  The other pointer should be NULL; otherwise this function will fail!
 * Upon success, `dst` will contain (dimx x dimy) doubles or floats as requested.
 * The caller of this function is responsible of free'ing `dst` using free().
 *
 * Return value meanings:
 * 0: success
 * 1: `dst` not pointing to a NULL pointer!
 * 2: `output_float` value not supported
 * -1: other errors
 */
int sperr_qzdecomp_2d(
    const void* src,      /* Input: buffer that contains a compressed bitstream */
    size_t src_len,       /* Input: length of the input bitstream in byte */
    int32_t output_float, /* Input: output data type: 1 == float, 0 == double */
    size_t* dimx,         /* Output: X (fast-varying) dimension */
    size_t* dimy,         /* Output: Y (slowest-varying) dimension */
    void** dst);          /* Output: buffer for the output 2D slice, allocated by this function */

/*
 * Compress a buffer that contains a 3D volume in fixed-error mode.
 *
 * The memory management is a little tricy; please read carefully!
 *
 * The output bitstream is stored in `dst`, which is a pointer pointing to another pointer
 * held by the caller.  The other pointer should be NULL; otherwise this function will fail!
 * Upon success, `dst` will contain a bitstream of length `dst_len`.
 * The caller of this function is responsible of free'ing `dst` using free().
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

/*
 * Decompress a 3D SPERR-compressed buffer in fixed-error mode.
 *
 * The memory management is a little tricy; please read carefully!
 *
 * The output 3D volume is stored in `dst`, which is a pointer pointing to another pointer
 * held by the caller.  The other pointer should be NULL; otherwise this function will fail!
 * Upon success, `dst` will contain (dimx x dimy x dimz) doubles or floats as requested.
 * The caller of this function is responsible of free'ing `dst` using free().
 *
 * Return value meanings:
 * 0: success
 * 1: `dst` not pointing to a NULL pointer!
 * 2: `output_float` value not supported
 * -1: other errors
 */
int sperr_qzdecomp_3d(
    const void* src,      /* Input: buffer that contains a compressed bitstream */
    size_t src_len,       /* Input: length of the input bitstream in byte */
    int32_t output_float, /* Input: output data type: 1 == float, 0 == double */
    int32_t nthreads, /* Input: number of OMP threads to use */
    size_t* dimx,         /* Output: X (fast-varying) dimension */
    size_t* dimy,         /* Output: Y dimension */
    size_t* dimz,         /* Output: Z (slowest-varying) dimension */
    void** dst);          /* Output: buffer for the output 3D slice, allocated by this function */

#else

/* fixed-size mode functions */

#endif

/*
 * Functions in both fixed-error and fixed-size mode.
 */

/* 
 * Given a SPERR bitstream, parse the header and retrieve various types of information.
 *
 * Return value meanings:
 * 0: success
 * 1: parsing error occured
 */
void sperr_parse_header(
    const void* ptr,         /* Input: the bitstream to parse */
    int32_t* version_major,  /* Output: major version number */
    int32_t* zstd_applied,   /* Output: if ZSTD applied (0 == no ZSTD; 1 == ZSTD applied) */
    int32_t* is_qz_term,     /* Output: compression mode (0 == fixed-size; 1 == fixed-error) */ 
    int32_t* is_3d,          /* Output: 3D volume or 2D slice (0 == 2D, 1 == 3D) */
    uint32_t* dim_x,         /* Output: X dimension */
    uint32_t* dim_y,         /* Output: Y dimension */
    uint32_t* dim_z);        /* Output: Z dimension */
 

#ifdef __cplusplus
} /* end of extern "C" */
} /* end of namespace C_API */
#endif

#endif
