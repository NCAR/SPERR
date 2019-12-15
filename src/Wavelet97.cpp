#include "Wavelet97.h"

#include <type_traits>
#include <cassert>

template< typename T >
int speck::Wavelet97::assign_data( const T* data, long x, long y, long z )
{
    static_assert( std::is_floating_point<T>::value, 
                   "!! Only floating point values are supported !!" );
    dim_x = x;
    dim_y = y;
    dim_z = z;
    long num_of_vals = dim_x * dim_y * dim_z;
    data_buf.reset( new double[ num_of_vals ] );
    for( long i = 0; i < num_of_vals; i++ )
        data_buf[i] = data[i];

    return 0;
}
template int speck::Wavelet97::assign_data( const float*  data, long x, long y, long z );
template int speck::Wavelet97::assign_data( const double* data, long x, long y, long z );


int speck::Wavelet97::dwt2d()
{
    //
    // Pre-process data
    //
    m_calc_num_of_levels();
    m_subtract_mean();

    return 0;
}

    
//
// Private Methods
//
void speck::Wavelet97::m_subtract_mean()
{
    assert( dim_x > 0 && dim_y > 0 && dim_z > 0 );

    //
    // Here we calculate row by row to avoid too big numbers.
    //
    std::unique_ptr<double[]> row_means( new double[ dim_y * dim_z ] );
    const double dim_x1 = 1.0 / double(dim_x);
    long counter1 = 0, counter2 = 0;
    for( long z = 0; z < dim_z; z++ )
        for( long y = 0; y < dim_y; y++ )
        {
            double sum = 0.0;
            for( long x = 0; x < dim_x; x++ )
                sum += data_buf[ counter1++ ];
            row_means[ counter2++ ] = sum * dim_x1;
        }

    std::unique_ptr<double[]> layer_means( new double[ dim_z ] );
    const double dim_y1 = 1.0 / double(dim_y);
    counter1 = 0; counter2 = 0;
    for( long z = 0; z < dim_z; z++ )
    {
        double sum = 0.0;
        for( long y = 0; y < dim_y; y++ )
            sum += row_means[ counter1++ ];
        layer_means[ counter2++ ] = sum * dim_y1;
    }

    double sum = 0.0;
    for( long z = 0; z < dim_z; z++ )
        sum += layer_means[ z ];

    data_mean = sum / double(dim_z);

    for( long i = 0; i < dim_x * dim_y * dim_z; i++ )
        data_buf[i] -= data_mean;
}

    
void speck::Wavelet97::m_dwt2d_one_level( double* plain, long len_x, long len_y )
{
    assert( len_x <= dim_x && len_y <= dim_y );

    // First perform DWT along X for every row
    if( len_x % 2 == 0 )    // Even length
    {
        for( long i = 0; i < len_y; i++ )
            this->QccWAVCDF97AnalysisSymmetricEvenEven( plain + i * dim_x, len_x );
    }
    else                    // Odd length
    {
        for( long i = 0; i < len_y; i++ )
            this->QccWAVCDF97AnalysisSymmetricOddEven(  plain + i * dim_x, len_x );
    }

    // Second perform DWT along Y for every column
    double tmp_buf[MAX_LEN];
    std::unique_ptr<double[]> tmp_ptr(nullptr);
    double* buf_ptr = tmp_buf;
    if( MAX_LEN < len_y )   // Need to allocate memory on the heap!
    {
        tmp_ptr.reset( new double[ len_y ] );
        buf_ptr = tmp_ptr.get();
    }

    if( len_y % 2 == 0 )    // Even length
    {
        for( long x = 0; x < len_x; x++ )
        {
            for( long y = 0; y < len_y; y++ )
                buf_ptr[y] = plain[ y * dim_x + x ];
            this->QccWAVCDF97AnalysisSymmetricEvenEven( buf_ptr, len_y );
            for( long y = 0; y < len_y; y++ )
                plain[ y * dim_x + x ] = buf_ptr[y];
        }
    }
    else                    // Odd length
    {
        for( long x = 0; x < len_x; x++ )
        {
            for( long y = 0; y < len_y; y++ )
                buf_ptr[y] = plain[ y * dim_x + x ];
            this->QccWAVCDF97AnalysisSymmetricOddEven( buf_ptr, len_y );
            for( long y = 0; y < len_y; y++ )
                plain[ y * dim_x + x ] = buf_ptr[y];
        }
    }
}
    

void speck::Wavelet97::m_calc_num_of_levels()
{
    assert( dim_x > 0 && dim_y > 0 && dim_z > 0 );
    auto min_xy = std::min( dim_x, dim_y );
    float f     = std::log2(float(min_xy) / 9.0f);
    level_xy    = f < 0.0f ? 0 : long(f) + 1;
    f           = std::log2( float(dim_z) / 9.0f );
    level_z     = f < 0.0f ? 0 : long(f) + 1;
}


long m_calc_approx_len( long orig_len, long lev )
{
    assert( lev > 0 );
    long low_len = orig_len;
    for( long i = 0; i < lev; i++ )
        low_len = low_len % 2 == 0 ? low_len / 2 : (low_len + 1) / 2;
    
    return low_len;
}


//
// Methods from QccPack
//
void speck::Wavelet97::QccWAVCDF97AnalysisSymmetricEvenEven( double* signal, 
                                                             long signal_length)
{
    long index;
    for (index = 1; index < signal_length - 2; index += 2)
        signal[index] += ALPHA * (signal[index - 1] + signal[index + 1]);

    signal[signal_length - 1] += 2.0 * ALPHA * signal[signal_length - 2];
    signal[0] += 2.0 * BETA * signal[1];

    for (index = 2; index < signal_length; index += 2)
        signal[index] += BETA * (signal[index + 1] + signal[index - 1]);

    for (index = 1; index < signal_length - 2; index += 2)
        signal[index] +=  GAMMA * (signal[index - 1] + signal[index + 1]);

    signal[signal_length - 1] += 2.0 * GAMMA * signal[signal_length - 2];
    signal[0] = EPSILON * (signal[0] + 2.0 * DELTA * signal[1]);

    for (index = 2; index < signal_length; index += 2)
        signal[index] =  EPSILON * (signal[index] + 
                         DELTA * (signal[index + 1] + signal[index - 1]));

    for (index = 1; index < signal_length; index += 2)
        signal[index] /= (-EPSILON);
}


void speck::Wavelet97::QccWAVCDF97SynthesisSymmetricEvenEven( double* signal, 
                                                              long signal_length)
{
    long index;
    for (index = 1; index < signal_length; index += 2)
        signal[index] *= (-EPSILON);

    signal[0] = signal[0]/EPSILON - 2.0 * DELTA * signal[1];

    for (index = 2; index < signal_length; index += 2)
        signal[index] = signal[index]/EPSILON - 
                        DELTA * (signal[index + 1] + signal[index - 1]);

    for (index = 1; index < signal_length - 2; index += 2)
        signal[index] -= GAMMA * (signal[index - 1] + signal[index + 1]);

    signal[signal_length - 1] -= 2.0 * GAMMA * signal[signal_length - 2];
    signal[0] -= 2.0 * BETA * signal[1];

    for (index = 2; index < signal_length; index += 2)
        signal[index] -= BETA * (signal[index + 1] + signal[index - 1]);

    for (index = 1; index < signal_length - 2; index += 2)
        signal[index] -= ALPHA * (signal[index - 1] + signal[index + 1]);

    signal[signal_length - 1] -= 2.0 * ALPHA * signal[signal_length - 2];
}


void speck::Wavelet97::QccWAVCDF97SynthesisSymmetricOddEven( double* signal,
                                                             long signal_length)
{
  long index;
  
  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] *= (-EPSILON);

  signal[0] = signal[0] / EPSILON - 2.0 * DELTA * signal[1];

  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] = signal[index] / EPSILON - 
                    DELTA * (signal[index + 1] + signal[index - 1]);

  signal[signal_length - 1] = signal[signal_length - 1] / EPSILON - 
                              2.0 * DELTA * signal[signal_length - 2];

  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] -= GAMMA * (signal[index - 1] + signal[index + 1]);

  signal[0] -= 2.0 * BETA * signal[1];

  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] -= BETA * (signal[index + 1] + signal[index - 1]);

  signal[signal_length - 1] -= 2.0 * BETA * signal[signal_length - 2];

  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] -= ALPHA * (signal[index - 1] + signal[index + 1]);
}


void speck::Wavelet97::QccWAVCDF97AnalysisSymmetricOddEven(double*  signal,
                                                           long signal_length)
{
  long index;

  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] += ALPHA * (signal[index - 1] + signal[index + 1]);
  
  signal[0] += 2.0 * BETA * signal[1];

  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] += BETA * (signal[index + 1] + signal[index - 1]);

  signal[signal_length - 1] += 2.0 * BETA * signal[signal_length - 2];

  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] += GAMMA * (signal[index - 1] + signal[index + 1]);
  
  signal[0] = EPSILON * (signal[0] + 2.0 * DELTA * signal[1]);

  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] = EPSILON * (signal[index] + 
                    DELTA * (signal[index + 1] + signal[index - 1]));

  signal[signal_length - 1] = EPSILON * (signal[signal_length - 1] +
                              2 * DELTA * signal[signal_length - 2]);

  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] /= (-EPSILON);
}


//
// For debug only 
//
const double* speck::Wavelet97::get_data() const
{
    return data_buf.get();
}
double speck::Wavelet97::get_mean() const
{
    return data_mean;
}
