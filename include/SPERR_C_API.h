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
namespace C {
extern "C" {
#endif

#ifdef QZ_TERM

/*
 * Compress a buffer that contains a 2D slice in float or double type.
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
    int32_t qlev,     /* Input: q (quantization) level for SPERR */
    double tol,       /* Input: tolerance for SPERR */
    void** dst,       /* Output: buffer for the output bitstream, allocated by this function */
    size_t* dst_len); /* Output: length of `dst` in byte */

/*
 * Decompress a 2D SPERR-compressed buffer.
 *
 * The memory management is a little tricy; please read carefully!
 *
 * The output 2D slice is stored in `dst`, which is a pointer pointing to another pointer
 * held by the caller.  The other pointer should be NULL; otherwise this function will fail!
 * Upon success, `dst` will contain (`dimx` x `dimy`) doubles or floats as requested.
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

#else

#endif

#ifdef __cplusplus
} /* end of extern "C" */
} /* end of namespace C */
#endif

#endif
