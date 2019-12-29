
/* Note: psnr is calculated in dB, and follows the equation described in:
   http://homepages.inf.ed.ac.uk/rbf/CVonline/LOCAL_COPIES/VELDHUIZEN/node18.html
   Also refer to https://www.mathworks.com/help/vision/ref/psnr.html
 */
int sam_get_stats( const float* arr1, const float* arr2, long len, /* input  */
                   float* rmse,       float* lmax,    float* psnr, /* output */
                   float* arr1min,    float* arr1max            ); /* output */


int sam_read_n_bytes( const char* filename, long n_bytes,             /* input  */
                      void*       buffer               );             /* output */

int sam_write_n_doubles( const char*   filename, long n_vals,         /* input  */
                         const double* buffer              );         /* input  */
