#include "CDF97.h"

#include <type_traits>
#include <cassert>

template< typename T >
int speck::CDF97::assign_data( const T* data, long x, long y, long z )
{
    static_assert( std::is_floating_point<T>::value, 
                   "!! Only floating point values are supported !!" );
    m_dim_x = x;
    m_dim_y = y;
    m_dim_z = z;
    long num_of_vals = m_dim_x * m_dim_y * m_dim_z;
    m_data_buf.reset( new double[ num_of_vals ] );
    for( long i = 0; i < num_of_vals; i++ )
        m_data_buf[i] = data[i];

    return 0;
}
template int speck::CDF97::assign_data( const float*  data, long x, long y, long z );
template int speck::CDF97::assign_data( const double* data, long x, long y, long z );


int speck::CDF97::dwt2d()
{
    //
    // Pre-process data
    //
    const auto level_xy = m_num_of_levels_xy();
    m_subtract_mean();

    for( long lev = 0; lev < level_xy; lev++ )
    {
        long len_x = m_calc_approx_len( m_dim_x, lev );
        long len_y = m_calc_approx_len( m_dim_y, lev );
        m_dwt2d_one_level( m_data_buf.get(), len_x, len_y );
    }

    return 0;
}

    
//
// Private Methods
//
void speck::CDF97::m_subtract_mean()
{
    assert( m_dim_x > 0 && m_dim_y > 0 && m_dim_z > 0 );

    //
    // Here we calculate mean row by row to avoid too big numbers.
    // (Not using kahan summation because that's hard to vectorize.
    //
    std::unique_ptr<double[]> row_means( new double[ m_dim_y * m_dim_z ] );
    const double dim_x1 = 1.0 / double(m_dim_x);
    long counter1 = 0, counter2 = 0;
    for( long z = 0; z < m_dim_z; z++ )
        for( long y = 0; y < m_dim_y; y++ )
        {
            double sum = 0.0;
            for( long x = 0; x < m_dim_x; x++ )
                sum += m_data_buf[ counter1++ ];
            row_means[ counter2++ ] = sum * dim_x1;
        }

    std::unique_ptr<double[]> layer_means( new double[ m_dim_z ] );
    const double dim_y1 = 1.0 / double(m_dim_y);
    counter1 = 0; counter2 = 0;
    for( long z = 0; z < m_dim_z; z++ )
    {
        double sum = 0.0;
        for( long y = 0; y < m_dim_y; y++ )
            sum += row_means[ counter1++ ];
        layer_means[ counter2++ ] = sum * dim_y1;
    }

    double sum = 0.0;
    for( long z = 0; z < m_dim_z; z++ )
        sum += layer_means[ z ];

    m_data_mean = sum / double(m_dim_z);

    for( long i = 0; i < m_dim_x * m_dim_y * m_dim_z; i++ )
        m_data_buf[i] -= m_data_mean;
}

    
void speck::CDF97::m_dwt2d_one_level( double* plane, long len_x, long len_y )
{
    assert( len_x <= m_dim_x && len_y <= m_dim_y );

    // First perform DWT along X for every row
    if( len_x % 2 == 0 )    // Even length
    {
        for( long i = 0; i < len_y; i++ )
            this->QccWAVCDF97AnalysisSymmetricEvenEven( plane + i * m_dim_x, len_x );
    }
    else                    // Odd length
    {
        for( long i = 0; i < len_y; i++ )
            this->QccWAVCDF97AnalysisSymmetricOddEven(  plane + i * m_dim_x, len_x );
    }

    // Second perform DWT along Y for every column
    double tmp_buf[BUF_LEN];
    std::unique_ptr<double[]> tmp_ptr(nullptr);
    double* buf_ptr = tmp_buf;
    if( BUF_LEN < len_y )   // Need to allocate memory on the heap!
    {
        tmp_ptr.reset( new double[ len_y ] );
        buf_ptr = tmp_ptr.get();
    }

    if( len_y % 2 == 0 )    // Even length
    {
        for( long x = 0; x < len_x; x++ )
        {
            for( long y = 0; y < len_y; y++ )
                buf_ptr[y] = plane[ y * m_dim_x + x ];
            this->QccWAVCDF97AnalysisSymmetricEvenEven( buf_ptr, len_y );
            for( long y = 0; y < len_y; y++ )
                plane[ y * m_dim_x + x ] = buf_ptr[y];
        }
    }
    else                    // Odd length
    {
        for( long x = 0; x < len_x; x++ )
        {
            for( long y = 0; y < len_y; y++ )
                buf_ptr[y] = plane[ y * m_dim_x + x ];
            this->QccWAVCDF97AnalysisSymmetricOddEven( buf_ptr, len_y );
            for( long y = 0; y < len_y; y++ )
                plane[ y * m_dim_x + x ] = buf_ptr[y];
        }
    }
}
    

long speck::CDF97::m_num_of_levels_xy() const
{
    assert( m_dim_x > 0 && m_dim_y > 0 );
    auto min_xy = std::min( m_dim_x, m_dim_y );
    float f     = std::log2(float(min_xy) / 9.0f);  // 9.0f for CDF 9/7 kernel
    long level_xy = f < 0.0f ? 0 : long(f) + 1;
    return level_xy;
}


long speck::CDF97::m_num_of_levels_z() const
{
    assert( m_dim_z > 0 );
    float f      = std::log2( float(m_dim_z) / 9.0f ); // 9.0f for CDF 9/7 kernel
    long level_z = f < 0.0f ? 0 : long(f) + 1;
    return level_z;
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
void speck::CDF97::QccWAVCDF97AnalysisSymmetricEvenEven( double* signal, 
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


void speck::CDF97::QccWAVCDF97SynthesisSymmetricEvenEven( double* signal, 
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


void speck::CDF97::QccWAVCDF97SynthesisSymmetricOddEven( double* signal,
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


void speck::CDF97::QccWAVCDF97AnalysisSymmetricOddEven(double*  signal,
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
const double* speck::CDF97::get_data() const
{
    return m_data_buf.get();
}
double speck::CDF97::get_mean() const
{
    return m_data_mean;
}
