#ifndef SAM_HELPERS_H
#define SAM_HELPERS_H

#include <stdlib.h>

#include "SpeckConfig.h"

/* 
   Note: psnr is calculated in dB, and follows the equation described in:
   http://homepages.inf.ed.ac.uk/rbf/CVonline/LOCAL_COPIES/VELDHUIZEN/node18.html
   Also refer to https://www.mathworks.com/help/vision/ref/psnr.html
 */
int sam_get_statsf( const float* arr1, const float* arr2, size_t len, /* input  */
                    float* rmse,       float* lmax,    float* psnr,   /* output */
                    float* arr1min,    float* arr1max            );   /* output */

int sam_get_statsd( const double* arr1, const double* arr2, size_t len,/* input  */
                    double* rmse,       double* lmax,   double* psnr, /* output */
                    double* arr1min,    double* arr1max            ); /* output */


int sam_read_n_bytes( const char* filename, size_t n_bytes,           /* input  */
                      void*       buffer               );             /* output */

int sam_write_n_doubles( const char*   filename, size_t n_vals,       /* input  */
                         const double* buffer              );         /* input  */


#endif 
