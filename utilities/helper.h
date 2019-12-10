
int get_rmse_lmax( const float* arr1, const float* arr2, long len, /* input  */
                   float* rmse,       float* lmax,                 /* output */
                   float* arr1min,    float* arr1max            ); /* output */


int read_n_bytes( const char* filename, long n_bytes,             /* input  */
                  void*       buffer               );             /* output */
