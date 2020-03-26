#include "CDF97.h"
#include "speck_helper.h"

#include <type_traits>
#include <cassert>
#include <cstring>  // for std::memcpy()

template< typename T >
void speck::CDF97::copy_data( const T* data, size_t len )
{
    static_assert( std::is_floating_point<T>::value, 
                   "!! Only floating point values are supported !!" );

    assert( len > 0 );
    size_t  tmp = m_dim_x * m_dim_y * m_dim_z;
    assert( tmp == 0 || tmp == len );
    m_buf_len = len;
    m_data_buf = std::make_unique<double[]>( len );
    for( size_t i = 0; i < len; i++ )
        m_data_buf[i] = data[i];
}
template void speck::CDF97::copy_data( const float*,  size_t );
template void speck::CDF97::copy_data( const double*, size_t );

template< typename T >
void speck::CDF97::copy_data( const T& data, size_t len )
{
    assert( len > 0 );
    size_t  tmp = m_dim_x * m_dim_y * m_dim_z;
    assert( tmp == 0 || tmp == len );
    m_buf_len = len;
    m_data_buf = std::make_unique<double[]>( len );
    for( size_t i = 0; i < len; i++ )
        m_data_buf[i] = data[i];
}
template void speck::CDF97::copy_data( const buffer_type_d&, size_t );
template void speck::CDF97::copy_data( const buffer_type_f&, size_t );


void speck::CDF97::take_data( std::unique_ptr<double[]> ptr )
{
    m_data_buf = std::move( ptr );
}


const speck::buffer_type_d& speck::CDF97::get_read_only_data() const
{
    return m_data_buf;
}


std::unique_ptr<double[]> speck::CDF97::release_data()
{
    return std::move( m_data_buf );
}


void speck::CDF97::dwt1d()
{
    m_calc_mean();
    for( size_t i = 0; i < m_buf_len; i++ )
        m_data_buf[i] -= m_data_mean;

    size_t num_xforms = speck::calc_num_of_xforms( m_dim_x );
    m_dwt1d( m_data_buf.get(), m_buf_len, num_xforms );
}


void speck::CDF97::idwt1d()
{
    size_t num_xforms = speck::calc_num_of_xforms( m_dim_x );
    m_idwt1d( m_data_buf.get(), m_buf_len, num_xforms );

    for( size_t i = 0; i < m_buf_len; i++ )
        m_data_buf[i] += m_data_mean;
}


void speck::CDF97::dwt2d()
{
    m_calc_mean();
    for( size_t i = 0; i < m_buf_len; i++ )
        m_data_buf[i] -= m_data_mean;

    size_t num_xforms_xy = speck::calc_num_of_xforms( std::min( m_dim_x, m_dim_y ) );
    m_dwt2d( m_data_buf.get(), num_xforms_xy );
}


void speck::CDF97::idwt2d()
{
    size_t num_xforms_xy = speck::calc_num_of_xforms( std::min( m_dim_x, m_dim_y ) );
    m_idwt2d( m_data_buf.get(), num_xforms_xy );

    for( size_t i = 0; i < m_buf_len; i++ )
        m_data_buf[i] += m_data_mean;
}


void speck::CDF97::dwt3d()
{
    m_calc_mean();
    for( size_t i = 0; i < m_buf_len; i++ )
        m_data_buf[i] -= m_data_mean;

    auto num_xforms_xy = speck::calc_num_of_xforms( std::min( m_dim_x, m_dim_y ) );

    // First transform along the Z dimension
    const size_t plane_size = m_dim_x * m_dim_y;
    auto num_xforms_z  = speck::calc_num_of_xforms( m_dim_z );
    buffer_type_d tmp  = std::make_unique< double[] >( m_dim_z * 2 );
    double* const ptr  = tmp.get();
    double* const ptr2 = ptr + m_dim_z;
    for( size_t offset = 0; offset < plane_size; offset++ )
    {
#if 0
        // Fill 1st half of the buffer
        for( size_t i = 0; i < m_dim_z; i++ )
            ptr[i] = m_data_buf[ offset + plane_size * i ];

        // Perform 1D DWT
        if( m_dim_z % 2 == 0 )
        {
            for( size_t i = 0; i < 
            this->QccWAVCDF97AnalysisSymmetricEvenEven( buf_ptr, len_x );
            m_gather_
        }

        
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
#endif


    }


    m_dwt2d( m_data_buf.get(), num_xforms_xy );
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
    buffer_type_d row_means = std::make_unique<double[]>( m_dim_y * m_dim_z );
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

    buffer_type_d layer_means = std::make_unique<double[]>( m_dim_z );
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


void speck::CDF97::m_dwt2d( double* plane, size_t num_of_lev )
{
    std::array<size_t, 2> len_x, len_y;
    for( size_t lev = 0; lev < num_of_lev; lev++ )
    {
        speck::calc_approx_detail_len( m_dim_x, lev, len_x );
        speck::calc_approx_detail_len( m_dim_y, lev, len_y );
        m_dwt2d_one_level( plane, len_x[0], len_y[0] );
    }
}


void speck::CDF97::m_idwt2d( double* plane, size_t num_of_lev )
{
    std::array<size_t, 2> len_x, len_y;
    for( size_t lev = num_of_lev; lev > 0; lev-- )
    {
        speck::calc_approx_detail_len( m_dim_x, lev - 1, len_x );
        speck::calc_approx_detail_len( m_dim_y, lev - 1, len_y );
        m_idwt2d_one_level( plane, len_x[0], len_y[0] );
    }
}


void speck::CDF97::m_dwt1d( double* array, size_t array_len, size_t num_of_lev,
                            double* tmp_buf                                   )
{
    double* ptr;
    buffer_type_d buf;
    if( tmp_buf != nullptr )
        ptr = tmp_buf;
    else
    {
        buf = std::make_unique< double[] >( array_len );
        ptr = buf.get();
    }
    
    std::array<size_t, 2> approx;
    for( size_t lev = 0; lev < num_of_lev; lev++ )
    {
        speck::calc_approx_detail_len( array_len, lev, approx );
        std::memcpy( ptr, array, sizeof(double) * approx[0] );
        if( approx[0] % 2 == 0 )
        {
            this->QccWAVCDF97AnalysisSymmetricEvenEven( ptr, approx[0] );
            m_gather_even( array, ptr, approx[0] );
        }
        else
        {
            this->QccWAVCDF97AnalysisSymmetricOddEven( ptr, approx[0] );
            m_gather_odd( array, ptr, approx[0] );
        }
    }
}


void speck::CDF97::m_idwt1d( double* array, size_t array_len, size_t num_of_lev,
                             double* tmp_buf                                   )
{
    double*       ptr;
    buffer_type_d buf;
    if( tmp_buf != nullptr )
        ptr = tmp_buf;
    else
    {
        buf = std::make_unique< double[] >( array_len );
        ptr = buf.get();
    }
    
    std::array<size_t, 2> approx;
    for( size_t lev = num_of_lev; lev > 0; lev-- )
    {
        speck::calc_approx_detail_len( array_len, lev - 1, approx );
        if( approx[0] % 2 == 0 )
        {
            m_scatter_even( ptr, array, approx[0] );
            this->QccWAVCDF97SynthesisSymmetricEvenEven( ptr, approx[0] );
        }
        else
        {
            m_scatter_odd( ptr, array, approx[0] );
            this->QccWAVCDF97SynthesisSymmetricOddEven( ptr, approx[0] );
        }
        std::memcpy( array, ptr, sizeof(double) * approx[0] );
    }
}
    

void speck::CDF97::m_dwt2d_one_level( double* plane, size_t len_x, size_t len_y )
{
    // Create temporary buffers to work on
    size_t len_xy = std::max( len_x, len_y );
    buffer_type_d buffer = std::make_unique<double[]>( len_xy * 2 );
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
    // Create temporary buffers to work on
    size_t len_xy = std::max( len_x, len_y );
    buffer_type_d buffer = std::make_unique<double[]>( len_xy * 2 );
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
    if( m_buf_len == 0 )
        m_buf_len = x * y * z;
    else
        assert( m_buf_len == x * y * z );
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
    
void speck::CDF97::reset()
{
    m_dim_x     = 0;
    m_dim_y     = 0;
    m_dim_z     = 0;
    m_buf_len   = 0;
    m_data_mean = 0.0;
    m_data_buf.reset();
}
