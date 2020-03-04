#include "CDF97.h"
#include "speck_helper.h"

#include <type_traits>
#include <cassert>
#include <cstring>  // for std::memcpy()

template< typename T >
void speck::CDF97::copy_data( const T* data )
{
    static_assert( std::is_floating_point<T>::value, 
                   "!! Only floating point values are supported !!" );

    assert( m_dim_x > 0 && m_dim_y > 0 && m_dim_z > 0 );
    size_t num_of_vals = m_dim_x * m_dim_y * m_dim_z;
    m_data_buf = std::make_unique<double[]>( num_of_vals );
    for( size_t i = 0; i < num_of_vals; i++ )
        m_data_buf[i] = data[i];
}
template void speck::CDF97::copy_data( const float*  );
template void speck::CDF97::copy_data( const double* );


void speck::CDF97::take_data( std::unique_ptr<double[]> ptr )
{
    m_data_buf = std::move( ptr );
}


const double* speck::CDF97::get_read_only_data() const
{
    return m_data_buf.get();
}


std::unique_ptr<double[]> speck::CDF97::release_data()
{
    return std::move( m_data_buf );
}


int speck::CDF97::dwt2d()
{
    //
    // Pre-process data
    //
    m_calc_mean();
    size_t num_of_vals = m_dim_x * m_dim_y ;
    for( size_t i = 0; i < num_of_vals; i++ )
        m_data_buf[i] -= m_data_mean;

    auto num_xforms_xy = speck::calc_num_of_xforms( std::min( m_dim_x, m_dim_y ) );
    for( size_t lev = 0; lev < num_xforms_xy; lev++ )
    {
        std::array<size_t, 2> len_x, len_y;
        speck::calc_approx_detail_len( m_dim_x, lev, len_x );
        speck::calc_approx_detail_len( m_dim_y, lev, len_y );
        m_dwt2d_one_level( m_data_buf.get(), len_x[0], len_y[0] );
    }

    return 0;
}


int speck::CDF97::idwt2d()
{
    size_t num_xforms_xy = speck::calc_num_of_xforms( std::min( m_dim_x, m_dim_y ) );
    if( num_xforms_xy > 0 )
    {
        for( size_t lev = num_xforms_xy; lev > 0; lev-- )
        {
            std::array<size_t, 2> len_x, len_y;
            speck::calc_approx_detail_len( m_dim_x, lev - 1, len_x );
            speck::calc_approx_detail_len( m_dim_y, lev - 1, len_y );
            m_idwt2d_one_level( m_data_buf.get(), len_x[0], len_y[0] );
        }
    }

    size_t num_of_vals = m_dim_x * m_dim_y ;
    for( size_t i = 0; i < num_of_vals; i++ )
        m_data_buf[i] += m_data_mean;

    return 0;
}

    
//
// Private Methods
//
void speck::CDF97::m_calc_mean()
{
    assert( m_dim_x > 0 && m_dim_y > 0 && m_dim_z > 0 );

    //
    // Here we calculate mean row by row to avoid too big numbers.
    // (Not using kahan summation because that's hard to vectorize.)
    //
    buffer_type row_means = std::make_unique<double[]>( m_dim_y * m_dim_z );
    const double dim_x1 = 1.0 / double(m_dim_x);
    size_t counter1 = 0, counter2 = 0;
    for( size_t z = 0; z < m_dim_z; z++ )
        for( size_t y = 0; y < m_dim_y; y++ )
        {
            double sum = 0.0;
            for( size_t x = 0; x < m_dim_x; x++ )
                sum += m_data_buf[ counter1++ ];
            row_means[ counter2++ ] = sum * dim_x1;
        }

    buffer_type layer_means = std::make_unique<double[]>( m_dim_z );
    const double dim_y1 = 1.0 / double(m_dim_y);
    counter1 = 0; counter2 = 0;
    for( size_t z = 0; z < m_dim_z; z++ )
    {
        double sum = 0.0;
        for( size_t y = 0; y < m_dim_y; y++ )
            sum += row_means[ counter1++ ];
        layer_means[ counter2++ ] = sum * dim_y1;
    }

    double sum = 0.0;
    for( size_t z = 0; z < m_dim_z; z++ )
        sum += layer_means[ z ];

    m_data_mean = sum / double(m_dim_z);
}

    
void speck::CDF97::m_dwt2d_one_level( double* plane, size_t len_x, size_t len_y )
{
    assert( len_x <= m_dim_x && len_y <= m_dim_y );

    // Create temporary buffers to work on
    size_t len_xy = std::max( len_x, len_y );
    buffer_type buffer = std::make_unique<double[]>( len_xy * 2 );
    double *const buf_ptr  = buffer.get();       // First half of the array
    double *const buf_ptr2 = buf_ptr + len_xy;   // Second half of the array

    // First, perform DWT along X for every row
    if( len_x % 2 == 0 )    // Even length
    {
        for( size_t i = 0; i < len_y; i++ )
        {
            auto* pos = plane + i * m_dim_x;
            std::memcpy( buf_ptr, pos, sizeof(double) * len_x );
            this->QccWAVCDF97AnalysisSymmetricEvenEven( buf_ptr, len_x );
            // pub back the resluts in low-pass and high-pass groups
            m_gather_even( pos, buf_ptr, len_x );
        }
    }
    else                    // Odd length
    {
        for( size_t i = 0; i < len_y; i++ )
        {
            auto* pos = plane + i * m_dim_x;
            std::memcpy( buf_ptr, pos, sizeof(double) * len_x );
            this->QccWAVCDF97AnalysisSymmetricOddEven( buf_ptr, len_x );
            // pub back the resluts in low-pass and high-pass groups
            m_gather_odd( pos, buf_ptr, len_x );
        }
    }

    // Second, perform DWT along Y for every column
    // Note, I've tested that up to 1024^2 planes it is actually slightly slower to 
    // transpose the plane and then perform the transforms.
    // This was consistent on both a MacBook and a RaspberryPi 3.

    if( len_y % 2 == 0 )    // Even length
    {
        for( size_t x = 0; x < len_x; x++ )
        {
            for( size_t y = 0; y < len_y; y++ )
                buf_ptr[y] = plane[ y * m_dim_x + x ];
            this->QccWAVCDF97AnalysisSymmetricEvenEven( buf_ptr, len_y );
            // Re-organize the resluts in low-pass and high-pass groups
            m_gather_even( buf_ptr2, buf_ptr, len_y );
            for( size_t y = 0; y < len_y; y++ )
                plane[ y * m_dim_x + x ] = buf_ptr2[y];
        }
    }
    else                    // Odd length
    {
        for( size_t x = 0; x < len_x; x++ )
        {
            for( size_t y = 0; y < len_y; y++ )
                buf_ptr[y] = plane[ y * m_dim_x + x ];
            this->QccWAVCDF97AnalysisSymmetricOddEven( buf_ptr, len_y );
            // Re-organize the resluts in low-pass and high-pass groups
            m_gather_odd( buf_ptr2, buf_ptr, len_y );
            for( size_t y = 0; y < len_y; y++ )
                plane[ y * m_dim_x + x ] = buf_ptr2[y];
        }
    }
}
    

void speck::CDF97::m_idwt2d_one_level( double* plane, size_t len_x, size_t len_y )
{
    assert( len_x <= m_dim_x && len_y <= m_dim_y );

    // Create temporary buffers to work on
    size_t len_xy = std::max( len_x, len_y );
    buffer_type buffer = std::make_unique<double[]>( len_xy * 2 );
    double *const buf_ptr  = buffer.get();       // First half of the array
    double *const buf_ptr2 = buf_ptr + len_xy;   // Second half of the array

    // First, perform IDWT along Y for every column
    if( len_y % 2 == 0 )    // Even length
    {
        for( size_t x = 0; x < len_x; x++ )
        {
            for( size_t y = 0; y < len_y; y++ )
                buf_ptr[y] = plane[ y * m_dim_x + x ];
            // Re-organize the coefficients as interleaved low-pass and high-pass ones
            m_scatter_even( buf_ptr2, buf_ptr, len_y );
            this->QccWAVCDF97SynthesisSymmetricEvenEven( buf_ptr2, len_y );
            for( size_t y = 0; y < len_y; y++ )
                plane[ y * m_dim_x + x ] = buf_ptr2[y];
        }
    }
    else                    // Odd length
    {
        for( size_t x = 0; x < len_x; x++ )
        {
            for( size_t y = 0; y < len_y; y++ )
                buf_ptr[y] = plane[ y * m_dim_x + x ];
            // Re-organize the coefficients as interleaved low-pass and high-pass ones
            m_scatter_odd( buf_ptr2, buf_ptr, len_y );
            this->QccWAVCDF97SynthesisSymmetricOddEven( buf_ptr2, len_y );
            for( size_t y = 0; y < len_y; y++ )
                plane[ y * m_dim_x + x ] = buf_ptr2[y];
        }
    }

    // Second, perform IDWT along X for every row
    if( len_x % 2 == 0 )    // Even length
    {
        for( size_t i = 0; i < len_y; i++ )
        {
            auto* pos = plane + i * m_dim_x;
            // Re-organize the coefficients as interleaved low-pass and high-pass ones
            m_scatter_even( buf_ptr, pos, len_x );
            this->QccWAVCDF97SynthesisSymmetricEvenEven( buf_ptr, len_x );
            std::memcpy( pos, buf_ptr, sizeof(double) * len_x );
        }
    }
    else                    // Odd length
    {
        for( size_t i = 0; i < len_y; i++ )
        {
            auto* pos = plane + i * m_dim_x;
            // Re-organize the coefficients as interleaved low-pass and high-pass ones
            m_scatter_odd( buf_ptr, pos, len_x );
            this->QccWAVCDF97SynthesisSymmetricOddEven( buf_ptr, len_x );
            std::memcpy( pos, buf_ptr, sizeof(double) * len_x );
        }
    }
}


void speck::CDF97::m_gather_even( double* dest, const double* orig, size_t len ) const
{
    assert( len % 2 == 0 ); // This function specifically for even length input
    size_t low_count = len / 2, high_count = len / 2; 
                            // How many low-pass and high-pass elements?
    size_t counter = 0;
    for( size_t i = 0; i < low_count; i++ )
        dest[counter++] = orig[i*2];
    for( size_t i = 0; i < high_count; i++ )
        dest[counter++] = orig[i*2+1];
}
void speck::CDF97::m_gather_odd( double* dest, const double* orig, size_t len ) const
{
    assert( len % 2 == 1 ); // This function specifically for odd length input
    size_t low_count = len / 2 + 1, high_count = len / 2; 
                            // How many low-pass and high-pass elements?
    size_t counter = 0;
    for( size_t i = 0; i < low_count; i++ )
        dest[counter++] = orig[i*2];
    for( size_t i = 0; i < high_count; i++ )
        dest[counter++] = orig[i*2+1];
}


void speck::CDF97::m_scatter_even( double* dest, const double* orig, size_t len ) const
{
    assert( len % 2 == 0 ); // This function specifically for even length input
    size_t low_count = len / 2, high_count = len / 2;
                            // How many low-pass and high-pass elements?
    size_t counter = 0;
    for( size_t i = 0; i < low_count; i++ )
        dest[i*2]   = orig[counter++];
    for( size_t i = 0; i < high_count; i++ )
        dest[i*2+1] = orig[counter++];
}
void speck::CDF97::m_scatter_odd( double* dest, const double* orig, size_t len ) const
{
    assert( len % 2 == 1 ); // This function specifically for odd length input
    size_t low_count = len / 2 + 1, high_count = len / 2;
                            // How many low-pass and high-pass elements?
    size_t counter = 0;
    for( size_t i = 0; i < low_count; i++ )
        dest[i*2]   = orig[counter++];
    for( size_t i = 0; i < high_count; i++ )
        dest[i*2+1] = orig[counter++];
}


//                 Z
//                /
//               /
//              /---------
//       cut_z /        /|
//            /        / |
//            |-------|  |---------X
//            |       |  /
//      cut_y |       | /
//            |       |/
//            |--------
//            |  cut_x
//            |
//            Y

void speck::CDF97::m_cut_transpose_XtoZ( double* dest,     size_t cut_len_x, 
                                         size_t cut_len_y, size_t cut_len_z ) const
{
    // This operation essentially swaps the X and Z indices, so we have
    const size_t dest_len_x = cut_len_z;
    const size_t dest_len_y = cut_len_y;
    const size_t dest_len_z = cut_len_x;

    const size_t plane_size = m_dim_x * m_dim_y;
    size_t counter = 0;
    for( size_t z = 0; z < dest_len_z; z++ )
        for( size_t y = 0; y < dest_len_y; y++ )
            for( size_t x = 0; x < dest_len_x; x++ )
            {
                size_t src_x   = z;
                size_t src_y   = y;
                size_t src_z   = x;
                size_t src_idx = src_z * plane_size + src_y * m_dim_x + src_x;
                dest[ counter++ ]  = m_data_buf[ src_idx ];
            }
}
void speck::CDF97::m_transpose_put_back_ZtoX( const double* buf, size_t len_x, 
                                              size_t len_y,      size_t len_z ) const
{
    // This operation essentially swaps the X and Z indices, so we have
    const size_t plane_size = m_dim_x * m_dim_y;
    size_t counter = 0;

    for( size_t z = 0; z < len_z; z++ )
        for( size_t y = 0; y < len_y; y++ )
            for( size_t x = 0; x < len_x; x++ )
            {
                auto dest_x   = z;
                auto dest_y   = y;
                auto dest_z   = x;
                auto dest_idx = dest_z * plane_size + dest_y * m_dim_x + dest_x;
                m_data_buf[ dest_idx ] = buf[ counter++ ];
            }
}

//
// Methods from QccPack
//
void speck::CDF97::QccWAVCDF97AnalysisSymmetricEvenEven( double* signal, size_t signal_length)
{
    size_t index;

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


void speck::CDF97::QccWAVCDF97SynthesisSymmetricEvenEven( double* signal, size_t signal_length)
{
    size_t index;

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


void speck::CDF97::QccWAVCDF97SynthesisSymmetricOddEven( double* signal, size_t signal_length)
{
  size_t index;
  
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


void speck::CDF97::QccWAVCDF97AnalysisSymmetricOddEven(double* signal, size_t signal_length)
{
  size_t index;

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


void speck::CDF97::set_mean( double m )
{
    m_data_mean = m;
}


void speck::CDF97::set_dims( size_t x, size_t y, size_t z )
{
    m_dim_x = x;
    m_dim_y = y;
    m_dim_z = z;
}


double speck::CDF97::get_mean() const
{
    return m_data_mean;
}


void speck::CDF97::get_dims( std::array<size_t, 2>& dims ) const
{
    dims[0] = m_dim_x;
    dims[1] = m_dim_y;
}


void speck::CDF97::get_dims( std::array<size_t, 3>& dims ) const
{
    dims[0] = m_dim_x;
    dims[1] = m_dim_y;
    dims[2] = m_dim_z;
}
