#include "CDF97.h"

#include <algorithm>
#include <cassert>
#include <cstring> // std::memcpy()
#include <numeric> // std::accumulate()
#include <type_traits>

#ifdef USE_OMP
   #include <omp.h>
#endif

template <typename T>
auto speck::CDF97::copy_data(const T* data, size_t len, size_t dimx, size_t dimy, size_t dimz) 
                 -> RTNType
{
    static_assert(std::is_floating_point<T>::value,
                  "!! Only floating point values are supported !!");
    if( len != dimx * dimy * dimz )
        return RTNType::DimMismatch;

    // If `m_data_buf` is empty, or having a different length, we need to allocate memory!
    if( m_data_buf == nullptr || m_buf_len != len ) {
        m_buf_len  = len;
        m_data_buf = std::make_unique<double[]>(len); 
    }
    std::copy( data, data + len, speck::begin(m_data_buf) );

    m_dim_x = dimx;
    m_dim_y = dimy;
    m_dim_z = dimz;

    auto max_col = std::max( std::max(dimx, dimy), dimz );
    // Notice that this buffer needs to hold two columns.
    if( !speck::size_is(m_col_buf, max_col * 2) ) {
        // Weird clang behavior (clang version 11.1.0 on MacOS, clang version 10.0.0-4ubuntu1)
        // that doesn't allow the following line to link.
        // m_col_buf = {std::make_unique<double[]>(max_col * 2), max_col * 2};
        m_col_buf = std::make_pair(std::make_unique<double[]>(max_col * 2), max_col * 2);
    }

    auto max_slice = std::max( std::max(dimx * dimy, dimx * dimz), dimy * dimz );
    if( !speck::size_is(m_slice_buf, max_slice) )
        m_slice_buf = {std::make_unique<double[]>(max_slice), max_slice};

    return RTNType::Good;
}
template auto speck::CDF97::copy_data(const float*,  size_t, size_t, size_t, size_t) -> RTNType;
template auto speck::CDF97::copy_data(const double*, size_t, size_t, size_t, size_t) -> RTNType;


auto speck::CDF97::take_data(buffer_type_d ptr, size_t len, size_t dimx, size_t dimy, size_t dimz)
                 -> RTNType
{
    if( len != dimx * dimy * dimz )
        return RTNType::DimMismatch;

    m_data_buf = std::move(ptr);
    m_buf_len  = len;
    m_dim_x    = dimx;
    m_dim_y    = dimy;
    m_dim_z    = dimz;

    auto max_col = std::max( std::max(dimx, dimy), dimz );
    if( !speck::size_is(m_col_buf, max_col * 2) )
        m_col_buf = {std::make_unique<double[]>(max_col * 2), max_col * 2};

    auto max_slice = std::max( std::max(dimx * dimy, dimx * dimz), dimy * dimz );
    if( !speck::size_is(m_slice_buf, max_slice) )
        m_slice_buf = {std::make_unique<double[]>(max_slice), max_slice};

    return RTNType::Good;
}

auto speck::CDF97::view_data() const -> std::pair<const double*, size_t>
{
    //return std::make_pair(std::cref(m_data_buf), m_buf_len);
    return std::make_pair(m_data_buf.get(), m_buf_len);
}

auto speck::CDF97::release_data() -> std::pair<buffer_type_d, size_t>
{
    const auto tmp = m_buf_len;
    m_buf_len = 0;
    return {std::move(m_data_buf), tmp};
}

void speck::CDF97::dwt1d()
{
    size_t num_xforms = speck::num_of_xforms(m_dim_x);
    m_dwt1d(m_data_buf.get(), m_buf_len, num_xforms, m_col_buf.first.get());
}

void speck::CDF97::idwt1d()
{
    size_t num_xforms = speck::num_of_xforms(m_dim_x);
    m_idwt1d(m_data_buf.get(), m_buf_len, num_xforms, m_col_buf.first.get());
}

void speck::CDF97::dwt2d()
{
    size_t num_xforms_xy = speck::num_of_xforms(std::min(m_dim_x, m_dim_y));
    m_dwt2d(m_data_buf.get(), m_dim_x, m_dim_y, num_xforms_xy, m_col_buf.first.get());
}

void speck::CDF97::idwt2d()
{
    size_t num_xforms_xy = speck::num_of_xforms(std::min(m_dim_x, m_dim_y));
    m_idwt2d(m_data_buf.get(), m_dim_x, m_dim_y, num_xforms_xy, m_col_buf.first.get());
}

void speck::CDF97::dwt3d()
{

#ifdef USE_OMP
    size_t num_threads = 1;
    #pragma omp parallel
    {
      if( omp_get_thread_num() == 0 )
        num_threads = omp_get_num_threads();
    }
    size_t max_dim              = std::max( std::max(m_dim_x, m_dim_y), m_dim_z );
    buffer_type_d tmp_buf_pool  = std::make_unique<double[]>(max_dim * 2 * num_threads);
    double* const tmp_buf       = tmp_buf_pool.get();
#else
    double* const tmp_buf       = m_col_buf.first.get();
#endif

    const size_t  plane_size_xy = m_dim_x * m_dim_y;

    /*
     *             Z
     *            /
     *           /
     *          /________
     *         /       /|
     *        /       / |
     *     0 |-------/-------> X
     *       |       |  |
     *       |       |  /
     *       |       | /
     *       |_______|/
     *       |
     *       |
     *       Y
     */

    /*
     * Note on the order of performing transforms in 3 dimensions:
     * this implementation follows QccPack's example.
     */

     // First transform along the Z dimension
     //

#ifdef USE_OMP
    buffer_type_d z_column_pool = std::make_unique<double[]>(m_dim_x * m_dim_z * num_threads);
    double* const z_columns_ptr = z_column_pool.get();
#else
    double* const z_columns_ptr = m_slice_buf.first.get();
#endif

    const auto    num_xforms_z   = speck::num_of_xforms(m_dim_z);

    // #pragma omp parallel for
    for (size_t y = 0; y < m_dim_y; y++) {
        const auto    y_offset    = y * m_dim_x;

#ifdef USE_OMP
        const size_t  my_rank     = omp_get_thread_num();
        double* const my_z_col    = z_columns_ptr + my_rank * m_dim_x * m_dim_z;
        double* const my_tmp_buf  = tmp_buf + my_rank * max_dim * 2;
#else
        double* const my_z_col    = z_columns_ptr;
        double* const my_tmp_buf  = tmp_buf;
#endif

        // Re-arrange values of one XZ slice so that they form many z_columns
        for (size_t z = 0; z < m_dim_z; z++) {
            const auto cube_start_idx = z * plane_size_xy + y_offset;
            for (size_t x = 0; x < m_dim_x; x++)
                my_z_col[z + x * m_dim_z] = m_data_buf[cube_start_idx + x];
        }

        // DWT1D on every z_column
        for (size_t x = 0; x < m_dim_x; x++)
            m_dwt1d(my_z_col + x * m_dim_z, m_dim_z, num_xforms_z, my_tmp_buf);

        // Put back values of the z_columns to the cube
        for (size_t z = 0; z < m_dim_z; z++) {
            const auto cube_start_idx = z * plane_size_xy + y_offset;
            for (size_t x = 0; x < m_dim_x; x++)
                m_data_buf[cube_start_idx + x] = my_z_col[z + x * m_dim_z];
        }
    }

    // Second transform each plane
    //
    const auto num_xforms_xy = speck::num_of_xforms(std::min(m_dim_x, m_dim_y));

    // #pragma omp parallel for
    for (size_t z = 0; z < m_dim_z; z++) {

#ifdef USE_OMP
        const size_t  my_rank     = omp_get_thread_num();
        double* const my_tmp_buf  = tmp_buf_pool.get() + my_rank * max_dim * 2;
#else
        double* const my_tmp_buf  = tmp_buf;
#endif

        const size_t  offset      = plane_size_xy * z;
        m_dwt2d(m_data_buf.get() + offset, m_dim_x, m_dim_y, num_xforms_xy, my_tmp_buf);
    }
}

void speck::CDF97::idwt3d()
{

#ifdef USE_OMP
    size_t num_threads = 1;
    #pragma omp parallel
    {
        if( omp_get_thread_num() == 0 )
            num_threads = omp_get_num_threads();
    }
    const size_t max_dim        = std::max( std::max(m_dim_x, m_dim_y), m_dim_z );
    buffer_type_d tmp_buf_pool  = std::make_unique<double[]>(max_dim * 2 * num_threads);
    double* const tmp_buf       = tmp_buf_pool.get();
#else
    double* const tmp_buf       = m_col_buf.first.get();
#endif

    const size_t  plane_size_xy = m_dim_x * m_dim_y;

    // First, inverse transform each plane
    //
    auto num_xforms_xy = speck::num_of_xforms(std::min(m_dim_x, m_dim_y));

    // #pragma omp parallel for
    for (size_t i = 0; i < m_dim_z; i++) {
        const size_t offset  = plane_size_xy * i;

#ifdef USE_OMP
        const size_t  my_rank    = omp_get_thread_num();
        double* const my_tmp_buf = tmp_buf + max_dim * 2 * my_rank;
#else
        double* const my_tmp_buf = tmp_buf;
#endif

        m_idwt2d(m_data_buf.get() + offset, m_dim_x, m_dim_y, num_xforms_xy, my_tmp_buf);
    }

    /*
     * Second, inverse transform along the Z dimension
     *
     *             Z
     *            /
     *           /
     *          /________
     *         /       /|
     *        /       / |
     *     0 |-------/-------> X
     *       |       |  |
     *       |       |  /
     *       |       | /
     *       |_______|/
     *       |
     *       |
     *       Y
     */

    // Process one XZ slice at a time
    //

#ifdef USE_OMP
    buffer_type_d z_column_pool = std::make_unique<double[]>(m_dim_x * m_dim_z * num_threads);
    double* const z_columns_ptr = z_column_pool.get();
#else
    double* const z_columns_ptr = m_slice_buf.first.get();
#endif

    const auto    num_xforms_z  = speck::num_of_xforms(m_dim_z);

    // #pragma omp parallel for
    for (size_t y = 0; y < m_dim_y; y++) {
        const auto y_offset  = y * m_dim_x;

#ifdef USE_OMP
        const size_t  my_rank    = omp_get_thread_num();
        double* const my_z_col   = z_columns_ptr + m_dim_x * m_dim_z * my_rank;
        double* const my_tmp_buf = tmp_buf + max_dim * 2 * my_rank;
#else
        double* const my_z_col   = z_columns_ptr;
        double* const my_tmp_buf = tmp_buf;
#endif

        // Re-arrange values on one slice so that they form many z_columns
        for (size_t z = 0; z < m_dim_z; z++) {
            const auto cube_start_idx = z * plane_size_xy + y_offset;
            for (size_t x = 0; x < m_dim_x; x++)
                my_z_col[z + x * m_dim_z] = m_data_buf[cube_start_idx + x];
        }

        // IDWT1D on every z_column
        for (size_t x = 0; x < m_dim_x; x++)
            m_idwt1d(my_z_col + x * m_dim_z, m_dim_z, num_xforms_z, my_tmp_buf);

        // Put back values from the z_columns to the cube
        for (size_t z = 0; z < m_dim_z; z++) {
            const auto cube_start_idx = z * plane_size_xy + y_offset;
            for (size_t x = 0; x < m_dim_x; x++)
                m_data_buf[cube_start_idx + x] = my_z_col[z + x * m_dim_z];
        }
    }
}


//
// Private Methods
//

void speck::CDF97::m_dwt2d(double* plane,
                           size_t  len_x,
                           size_t  len_y,
                           size_t  num_of_lev,
                           double* tmp_buf)
{
    std::array<size_t, 2> approx_x, approx_y;
    for (size_t lev = 0; lev < num_of_lev; lev++) {
        speck::calc_approx_detail_len(len_x, lev, approx_x);
        speck::calc_approx_detail_len(len_y, lev, approx_y);
        m_dwt2d_one_level(plane, approx_x[0], approx_y[0], tmp_buf);
    }
}

void speck::CDF97::m_idwt2d(double* plane,
                            size_t  len_x,
                            size_t  len_y,
                            size_t  num_of_lev,
                            double* tmp_buf)
{
    std::array<size_t, 2> approx_x, approx_y;
    for (size_t lev = num_of_lev; lev > 0; lev--) {
        speck::calc_approx_detail_len(len_x, lev - 1, approx_x);
        speck::calc_approx_detail_len(len_y, lev - 1, approx_y);
        m_idwt2d_one_level(plane, approx_x[0], approx_y[0], tmp_buf);
    }
}

void speck::CDF97::m_dwt1d(double* array,
                           size_t  array_len,
                           size_t  num_of_lev,
                           double* tmp_buf)
{
    double* const ptr = tmp_buf;
    std::array<size_t, 2> approx;
    for (size_t lev = 0; lev < num_of_lev; lev++) {
        speck::calc_approx_detail_len(array_len, lev, approx);
        std::memcpy(ptr, array, sizeof(double) * approx[0]);
        if (approx[0] % 2 == 0) {
            this->QccWAVCDF97AnalysisSymmetricEvenEven(ptr, approx[0]);
            m_gather_even(array, ptr, approx[0]);
        } else {
            this->QccWAVCDF97AnalysisSymmetricOddEven(ptr, approx[0]);
            m_gather_odd(array, ptr, approx[0]);
        }
    }
}

void speck::CDF97::m_idwt1d(double* array,
                            size_t  array_len,
                            size_t  num_of_lev,
                            double* tmp_buf)
{
    double* const ptr = tmp_buf;
    std::array<size_t, 2> approx;
    for (size_t lev = num_of_lev; lev > 0; lev--) {
        speck::calc_approx_detail_len(array_len, lev - 1, approx);
        if (approx[0] % 2 == 0) {
            m_scatter_even(ptr, array, approx[0]);
            this->QccWAVCDF97SynthesisSymmetricEvenEven(ptr, approx[0]);
        } else {
            m_scatter_odd(ptr, array, approx[0]);
            this->QccWAVCDF97SynthesisSymmetricOddEven(ptr, approx[0]);
        }
        std::memcpy(array, ptr, sizeof(double) * approx[0]);
    }
}

void speck::CDF97::m_dwt2d_one_level(double* plane,
                                     size_t  len_x,
                                     size_t  len_y,
                                     double* tmp_buf)
{
    // Specify temporary buffers to work on
    size_t        len_xy   = std::max(len_x, len_y);
    double* const buf_ptr  = tmp_buf;          // First half of the array
    double* const buf_ptr2 = tmp_buf + len_xy; // Second half of the array

    // First, perform DWT along X for every row
    if (len_x % 2 == 0) // Even length
    {
        for (size_t i = 0; i < len_y; i++) {
            auto* pos = plane + i * m_dim_x;
            std::memcpy(buf_ptr, pos, sizeof(double) * len_x);
            this->QccWAVCDF97AnalysisSymmetricEvenEven(buf_ptr, len_x);
            // pub back the resluts in low-pass and high-pass groups
            m_gather_even(pos, buf_ptr, len_x);
        }
    } else // Odd length
    {
        for (size_t i = 0; i < len_y; i++) {
            auto* pos = plane + i * m_dim_x;
            std::memcpy(buf_ptr, pos, sizeof(double) * len_x);
            this->QccWAVCDF97AnalysisSymmetricOddEven(buf_ptr, len_x);
            // pub back the resluts in low-pass and high-pass groups
            m_gather_odd(pos, buf_ptr, len_x);
        }
    }

    // Second, perform DWT along Y for every column
    // Note, I've tested that up to 1024^2 planes it is actually slightly slower
    // to transpose the plane and then perform the transforms. This was consistent
    // on both a MacBook and a RaspberryPi 3. Note2, I've tested transpose again
    // on an X86 linux machine using gcc, clang, and pgi. Again the difference is
    // either indistinguishable, or the current implementation has a slight edge.

    if (len_y % 2 == 0) // Even length
    {
        for (size_t x = 0; x < len_x; x++) {
            for (size_t y = 0; y < len_y; y++)
                buf_ptr[y] = plane[y * m_dim_x + x];
            this->QccWAVCDF97AnalysisSymmetricEvenEven(buf_ptr, len_y);
            // Re-organize the resluts in low-pass and high-pass groups
            m_gather_even(buf_ptr2, buf_ptr, len_y);
            for (size_t y = 0; y < len_y; y++)
                plane[y * m_dim_x + x] = buf_ptr2[y];
        }
    } else // Odd length
    {
        for (size_t x = 0; x < len_x; x++) {
            for (size_t y = 0; y < len_y; y++)
                buf_ptr[y] = plane[y * m_dim_x + x];
            this->QccWAVCDF97AnalysisSymmetricOddEven(buf_ptr, len_y);
            // Re-organize the resluts in low-pass and high-pass groups
            m_gather_odd(buf_ptr2, buf_ptr, len_y);
            for (size_t y = 0; y < len_y; y++)
                plane[y * m_dim_x + x] = buf_ptr2[y];
        }
    }
}

void speck::CDF97::m_idwt2d_one_level(double* plane,
                                      size_t  len_x,
                                      size_t  len_y,
                                      double* tmp_buf)
{
    // Specify temporary buffers to work on
    size_t        len_xy   = std::max(len_x, len_y);
    double* const buf_ptr  = tmp_buf;          // First half of the array
    double* const buf_ptr2 = tmp_buf + len_xy; // Second half of the array

    // First, perform IDWT along Y for every column
    if (len_y % 2 == 0) // Even length
    {
        for (size_t x = 0; x < len_x; x++) {
            for (size_t y = 0; y < len_y; y++)
                buf_ptr[y] = plane[y * m_dim_x + x];
            // Re-organize the coefficients as interleaved low-pass and high-pass ones
            m_scatter_even(buf_ptr2, buf_ptr, len_y);
            this->QccWAVCDF97SynthesisSymmetricEvenEven(buf_ptr2, len_y);
            for (size_t y = 0; y < len_y; y++)
                plane[y * m_dim_x + x] = buf_ptr2[y];
        }
    } else // Odd length
    {
        for (size_t x = 0; x < len_x; x++) {
            for (size_t y = 0; y < len_y; y++)
                buf_ptr[y] = plane[y * m_dim_x + x];
            // Re-organize the coefficients as interleaved low-pass and high-pass ones
            m_scatter_odd(buf_ptr2, buf_ptr, len_y);
            this->QccWAVCDF97SynthesisSymmetricOddEven(buf_ptr2, len_y);
            for (size_t y = 0; y < len_y; y++)
                plane[y * m_dim_x + x] = buf_ptr2[y];
        }
    }

    // Second, perform IDWT along X for every row
    if (len_x % 2 == 0) // Even length
    {
        for (size_t i = 0; i < len_y; i++) {
            auto* pos = plane + i * m_dim_x;
            // Re-organize the coefficients as interleaved low-pass and high-pass ones
            m_scatter_even(buf_ptr, pos, len_x);
            this->QccWAVCDF97SynthesisSymmetricEvenEven(buf_ptr, len_x);
            std::memcpy(pos, buf_ptr, sizeof(double) * len_x);
        }
    } else // Odd length
    {
        for (size_t i = 0; i < len_y; i++) {
            auto* pos = plane + i * m_dim_x;
            // Re-organize the coefficients as interleaved low-pass and high-pass ones
            m_scatter_odd(buf_ptr, pos, len_x);
            this->QccWAVCDF97SynthesisSymmetricOddEven(buf_ptr, len_x);
            std::memcpy(pos, buf_ptr, sizeof(double) * len_x);
        }
    }
}

void speck::CDF97::m_gather_even(double* dest, const double* orig, size_t len) const
{
    assert(len % 2 == 0); // This function specifically for even length input
    size_t low_count = len / 2, high_count = len / 2;
    // How many low-pass and high-pass elements?
    size_t counter = 0;
    for (size_t i = 0; i < low_count; i++)
        dest[counter++] = orig[i * 2];
    for (size_t i = 0; i < high_count; i++)
        dest[counter++] = orig[i * 2 + 1];
}
void speck::CDF97::m_gather_odd(double* dest, const double* orig, size_t len) const
{
    assert(len % 2 == 1); // This function specifically for odd length input
    size_t low_count = len / 2 + 1, high_count = len / 2;
    // How many low-pass and high-pass elements?
    size_t counter = 0;
    for (size_t i = 0; i < low_count; i++)
        dest[counter++] = orig[i * 2];
    for (size_t i = 0; i < high_count; i++)
        dest[counter++] = orig[i * 2 + 1];
}

void speck::CDF97::m_scatter_even(double* dest, const double* orig, size_t len) const
{
    assert(len % 2 == 0); // This function specifically for even length input
    size_t low_count = len / 2, high_count = len / 2;
    // How many low-pass and high-pass elements?
    size_t counter = 0;
    for (size_t i = 0; i < low_count; i++)
        dest[i * 2] = orig[counter++];
    for (size_t i = 0; i < high_count; i++)
        dest[i * 2 + 1] = orig[counter++];
}
void speck::CDF97::m_scatter_odd(double* dest, const double* orig, size_t len) const
{
    assert(len % 2 == 1); // This function specifically for odd length input
    size_t low_count = len / 2 + 1, high_count = len / 2;
    // How many low-pass and high-pass elements?
    size_t counter = 0;
    for (size_t i = 0; i < low_count; i++)
        dest[i * 2] = orig[counter++];
    for (size_t i = 0; i < high_count; i++)
        dest[i * 2 + 1] = orig[counter++];
}

//
// Methods from QccPack
//
void speck::CDF97::QccWAVCDF97AnalysisSymmetricEvenEven(double* signal,
                                                        size_t  signal_length)
{
    for (size_t index = 1; index < signal_length - 2; index += 2)
        signal[index] += ALPHA * (signal[index - 1] + signal[index + 1]);

    signal[signal_length - 1] += 2.0 * ALPHA * signal[signal_length - 2];
    signal[0] += 2.0 * BETA * signal[1];

    for (size_t index = 2; index < signal_length; index += 2)
        signal[index] += BETA * (signal[index + 1] + signal[index - 1]);

    for (size_t index = 1; index < signal_length - 2; index += 2)
        signal[index] += GAMMA * (signal[index - 1] + signal[index + 1]);

    signal[signal_length - 1] += 2.0 * GAMMA * signal[signal_length - 2];
    signal[0] = EPSILON * (signal[0] + 2.0 * DELTA * signal[1]);

    for (size_t index = 2; index < signal_length; index += 2)
        signal[index] = EPSILON * (signal[index] + DELTA * (signal[index + 1] + signal[index - 1]));

    for (size_t index = 1; index < signal_length; index += 2)
        signal[index] *= -INV_EPSILON;
}

void speck::CDF97::QccWAVCDF97SynthesisSymmetricEvenEven(double* signal,
                                                         size_t  signal_length)
{
    for (size_t index = 1; index < signal_length; index += 2)
        signal[index] *= (-EPSILON);

    signal[0] = signal[0] * INV_EPSILON - 2.0 * DELTA * signal[1];

    for (size_t index = 2; index < signal_length; index += 2)
        signal[index] = signal[index] * INV_EPSILON - DELTA * (signal[index + 1] + signal[index - 1]);

    for (size_t index = 1; index < signal_length - 2; index += 2)
        signal[index] -= GAMMA * (signal[index - 1] + signal[index + 1]);

    signal[signal_length - 1] -= 2.0 * GAMMA * signal[signal_length - 2];
    signal[0] -= 2.0 * BETA * signal[1];

    for (size_t index = 2; index < signal_length; index += 2)
        signal[index] -= BETA * (signal[index + 1] + signal[index - 1]);

    for (size_t index = 1; index < signal_length - 2; index += 2)
        signal[index] -= ALPHA * (signal[index - 1] + signal[index + 1]);

    signal[signal_length - 1] -= 2.0 * ALPHA * signal[signal_length - 2];
}

void speck::CDF97::QccWAVCDF97SynthesisSymmetricOddEven(double* signal,
                                                        size_t  signal_length)
{
    for (size_t index = 1; index < signal_length - 1; index += 2)
        signal[index] *= (-EPSILON);

    signal[0] = signal[0] * INV_EPSILON - 2.0 * DELTA * signal[1];

    for (size_t index = 2; index < signal_length - 2; index += 2)
        signal[index] = signal[index] * INV_EPSILON - DELTA * (signal[index + 1] + signal[index - 1]);

    signal[signal_length - 1] = signal[signal_length - 1] * INV_EPSILON - 
                                2.0 * DELTA * signal[signal_length - 2];

    for (size_t index = 1; index < signal_length - 1; index += 2)
        signal[index] -= GAMMA * (signal[index - 1] + signal[index + 1]);

    signal[0] -= 2.0 * BETA * signal[1];

    for (size_t index = 2; index < signal_length - 2; index += 2)
        signal[index] -= BETA * (signal[index + 1] + signal[index - 1]);

    signal[signal_length - 1] -= 2.0 * BETA * signal[signal_length - 2];

    for (size_t index = 1; index < signal_length - 1; index += 2)
        signal[index] -= ALPHA * (signal[index - 1] + signal[index + 1]);
}

void speck::CDF97::QccWAVCDF97AnalysisSymmetricOddEven(double* signal,
                                                       size_t  signal_length)
{
    for (size_t index = 1; index < signal_length - 1; index += 2)
        signal[index] += ALPHA * (signal[index - 1] + signal[index + 1]);

    signal[0] += 2.0 * BETA * signal[1];

    for (size_t index = 2; index < signal_length - 2; index += 2)
        signal[index] += BETA * (signal[index + 1] + signal[index - 1]);

    signal[signal_length - 1] += 2.0 * BETA * signal[signal_length - 2];

    for (size_t index = 1; index < signal_length - 1; index += 2)
        signal[index] += GAMMA * (signal[index - 1] + signal[index + 1]);

    signal[0] = EPSILON * (signal[0] + 2.0 * DELTA * signal[1]);

    for (size_t index = 2; index < signal_length - 2; index += 2)
        signal[index] = EPSILON * (signal[index] + DELTA * (signal[index + 1] + signal[index - 1]));

    signal[signal_length - 1] = EPSILON * (signal[signal_length - 1] + 
                                2.0 * DELTA * signal[signal_length - 2]);

    for (size_t index = 1; index < signal_length - 1; index += 2)
        signal[index] *= (-INV_EPSILON);
}


auto speck::CDF97::get_dims() const -> std::array<size_t, 3>
{
    return {m_dim_x, m_dim_y, m_dim_z};
}

