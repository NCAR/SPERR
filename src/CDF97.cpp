#include "CDF97.h"

#include <algorithm>
#include <cassert>
#include <cstring> // for std::memcpy()
#include <numeric> // for std::accumulate()
#include <type_traits>

#ifdef USE_OMP
    #include <omp.h>
#endif

template <typename T>
void speck::CDF97::copy_data(const T* data, size_t len)
{
    static_assert(std::is_floating_point<T>::value,
                  "!! Only floating point values are supported !!");

    assert(m_dim_x * m_dim_y * m_dim_z == 0 || m_dim_x * m_dim_y * m_dim_z == len);
    m_buf_len  = len;
    m_data_buf = std::make_unique<double[]>(len);
    std::copy( data, data + len, speck::begin(m_data_buf) );
}
template void speck::CDF97::copy_data(const float*, size_t);
template void speck::CDF97::copy_data(const double*, size_t);

void speck::CDF97::take_data(buffer_type_d ptr, size_t len)
{
    assert(m_dim_x * m_dim_y * m_dim_z == 0 || m_dim_x * m_dim_y * m_dim_z == len);
    m_buf_len  = len;
    m_data_buf = std::move(ptr);
}

auto speck::CDF97::get_read_only_data() const -> std::pair<const buffer_type_d&, size_t>
{
    return std::make_pair(std::cref(m_data_buf), m_buf_len);
    // Note: the following syntax would also work, but the code above better expresses intent. 
    // return {m_data_buf, m_buf_len};
}

auto speck::CDF97::release_data() -> std::pair<buffer_type_d, size_t>
{
    const auto tmp = m_buf_len;
    m_buf_len = 0;
    return {std::move(m_data_buf), tmp};
}

void speck::CDF97::dwt1d()
{
    m_calc_mean();
    auto m_data_begin = speck::begin( m_data_buf );
    std::for_each( m_data_begin, m_data_begin + m_buf_len,
                   [tmp = m_data_mean](auto& val){val -= tmp;} );

    size_t num_xforms = speck::num_of_xforms(m_dim_x);
    m_dwt1d(m_data_buf.get(), m_buf_len, num_xforms);
}

void speck::CDF97::idwt1d()
{
    size_t num_xforms = speck::num_of_xforms(m_dim_x);
    m_idwt1d(m_data_buf.get(), m_buf_len, num_xforms);

    auto m_data_begin = speck::begin( m_data_buf );
    std::for_each( m_data_begin, m_data_begin + m_buf_len,
                   [tmp = m_data_mean](auto& val){val += tmp;} );
}

void speck::CDF97::dwt2d()
{
    m_calc_mean();
    auto m_data_begin = speck::begin( m_data_buf );
    std::for_each( m_data_begin, m_data_begin + m_buf_len,
                   [tmp = m_data_mean](auto& val){val -= tmp;} );

    size_t     num_xforms_xy = speck::num_of_xforms(std::min(m_dim_x, m_dim_y));
    const auto max_dim       = std::max(m_dim_x, m_dim_y);
    auto       tmp_buf       = std::make_unique<double[]>(max_dim * 2);
    m_dwt2d(m_data_buf.get(), m_dim_x, m_dim_y, num_xforms_xy, tmp_buf.get());
}

void speck::CDF97::idwt2d()
{
    const auto max_dim       = std::max(m_dim_x, m_dim_y);
    auto       tmp_buf       = std::make_unique<double[]>(max_dim * 2);
    size_t     num_xforms_xy = speck::num_of_xforms(std::min(m_dim_x, m_dim_y));
    m_idwt2d(m_data_buf.get(), m_dim_x, m_dim_y, num_xforms_xy, tmp_buf.get());

    auto m_data_begin = speck::begin( m_data_buf );
    std::for_each( m_data_begin, m_data_begin + m_buf_len,
                   [tmp = m_data_mean](auto& val){val += tmp;} );
}

void speck::CDF97::dwt3d()
{
    m_calc_mean();
    auto m_data_begin = speck::begin( m_data_buf );
    std::for_each( m_data_begin, m_data_begin + m_buf_len,
                   [tmp = m_data_mean](auto& val){val -= tmp;} );

    size_t num_threads = 1;
#ifdef USE_OMP
    #pragma omp parallel
    {
      if( omp_get_thread_num() == 0 )
        num_threads = omp_get_num_threads();
    }
#endif

    size_t max_dim              = std::max(m_dim_x, m_dim_y);
    max_dim                     = std::max(max_dim, m_dim_z);
    buffer_type_d tmp_buf_pool  = std::make_unique<double[]>(max_dim * 2 * num_threads);
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
     *
     * First transform along the Z dimension
     */

    // Process one XZ slice at a time
    buffer_type_d z_column_pool = std::make_unique<double[]>(m_dim_x * m_dim_z * num_threads);
    const auto    num_xforms_z   = speck::num_of_xforms(m_dim_z);

    #pragma omp parallel for
    for (size_t y = 0; y < m_dim_y; y++) {
        const auto    y_offset    = y * m_dim_x;
#ifdef USE_OMP
        const size_t  my_rank     = omp_get_thread_num();
#else
        const size_t  my_rank     = 0;
#endif
        double* const my_z_col    = z_column_pool.get() + my_rank * m_dim_x * m_dim_z;
        double* const my_tmp_buf  = tmp_buf_pool.get() + my_rank * max_dim * 2;

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
    const auto num_xforms_xy = speck::num_of_xforms(std::min(m_dim_x, m_dim_y));

    #pragma omp parallel for
    for (size_t z = 0; z < m_dim_z; z++) {
#ifdef USE_OMP
        const size_t  my_rank     = omp_get_thread_num();
#else
        const size_t  my_rank     = 0;
#endif
        double* const my_tmp_buf  = tmp_buf_pool.get() + my_rank * max_dim * 2;
        const size_t  offset      = plane_size_xy * z;
        m_dwt2d(m_data_buf.get() + offset, m_dim_x, m_dim_y, num_xforms_xy, my_tmp_buf);
    }
}

void speck::CDF97::idwt3d()
{
    size_t num_threads = 1;
#ifdef USE_OMP
    #pragma omp parallel
    {
        if( omp_get_thread_num() == 0 )
            num_threads = omp_get_num_threads();
    }
#endif

    size_t max_dim              = std::max(m_dim_x, m_dim_y);
    max_dim                     = std::max(max_dim, m_dim_z);
    buffer_type_d tmp_buf_pool  = std::make_unique<double[]>(max_dim * 2 * num_threads);
    const size_t  plane_size_xy = m_dim_x * m_dim_y;

    // First, inverse transform each plane
    auto num_xforms_xy = speck::num_of_xforms(std::min(m_dim_x, m_dim_y));

    #pragma omp parallel for
    for (size_t i = 0; i < m_dim_z; i++) {
        const size_t offset  = plane_size_xy * i;
#ifdef USE_OMP
        const size_t my_rank = omp_get_thread_num();
#else
        const size_t my_rank = 0;
#endif
        double* const my_tmp_buf = tmp_buf_pool.get() + max_dim * 2 * my_rank;
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
    buffer_type_d z_column_pool = std::make_unique<double[]>(m_dim_x * m_dim_z * num_threads);
    const auto    num_xforms_z  = speck::num_of_xforms(m_dim_z);

    #pragma omp parallel for
    for (size_t y = 0; y < m_dim_y; y++) {
        const auto y_offset  = y * m_dim_x;
#ifdef USE_OMP
        const size_t my_rank = omp_get_thread_num();
#else
        const size_t my_rank = 0;
#endif
        double* const my_z_col   = z_column_pool.get() + m_dim_x * m_dim_z * my_rank;
        double* const my_tmp_buf = tmp_buf_pool.get() + max_dim * 2 * my_rank;

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

    // Finally, add back the mean which was subtracted earlier.
    auto m_data_begin = speck::begin( m_data_buf );
    std::for_each( m_data_begin, m_data_begin + m_buf_len,
                   [tmp = m_data_mean](auto& val){val += tmp;} );
}


//
// Private Methods
//
void speck::CDF97::m_calc_mean()
{
    //
    // Here we calculate mean slice by slice to avoid too big sums.
    // One test shows that this implementation is 4X faster than Kahan algorithm
    // Another test shows that OpenMP actually slows down this calculation...
    //
    assert(m_dim_x > 0 && m_dim_y > 0 && m_dim_z > 0);

    auto slice_means        = std::make_unique<double[]>(m_dim_z);
    const size_t slice_size = m_dim_x * m_dim_y;

    for (size_t z = 0; z < m_dim_z; z++) {
        auto begin = speck::begin( m_data_buf ) + slice_size * z;
        auto end   = begin + slice_size;
        slice_means[z] = std::accumulate( begin, end, double{0.0} ) / double(slice_size);
    }

    auto begin = speck::begin( slice_means );
    double sum = std::accumulate( begin, begin + m_dim_z, double{0.0} );

    m_data_mean = sum / double(m_dim_z);
}

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
    double*       ptr;
    buffer_type_d buf;
    if (tmp_buf != nullptr) {
        ptr = tmp_buf;
    } else {
        buf = std::make_unique<double[]>(array_len);
        ptr = buf.get();
    }

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
    double*       ptr;
    buffer_type_d buf;
    if (tmp_buf != nullptr) {
        ptr = tmp_buf;
    } else {
        buf = std::make_unique<double[]>(array_len);
        ptr = buf.get();
    }

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
    // Create/specify temporary buffers to work on
    size_t        len_xy = std::max(len_x, len_y);
    double *      buf_ptr, *buf_ptr2;
    buffer_type_d buffer;
    if (tmp_buf != nullptr) {
        buf_ptr  = tmp_buf;          // First half of the array
        buf_ptr2 = tmp_buf + len_xy; // Second half of the array
    } else {
        buffer   = std::make_unique<double[]>(len_xy * 2);
        buf_ptr  = buffer.get();     // First half of the array
        buf_ptr2 = buf_ptr + len_xy; // Second half of the array
    }

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
    // Create/specify temporary buffers to work on
    size_t        len_xy = std::max(len_x, len_y);
    double *      buf_ptr, *buf_ptr2;
    buffer_type_d buffer;
    if (tmp_buf != nullptr) {
        buf_ptr  = tmp_buf;          // First half of the array
        buf_ptr2 = tmp_buf + len_xy; // Second half of the array
    } else {
        buffer   = std::make_unique<double[]>(len_xy * 2);
        buf_ptr  = buffer.get();     // First half of the array
        buf_ptr2 = buf_ptr + len_xy; // Second half of the array
    }

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
    size_t index;

    for (index = 1; index < signal_length - 2; index += 2)
        signal[index] += ALPHA * (signal[index - 1] + signal[index + 1]);

    signal[signal_length - 1] += 2.0 * ALPHA * signal[signal_length - 2];
    signal[0] += 2.0 * BETA * signal[1];

    for (index = 2; index < signal_length; index += 2)
        signal[index] += BETA * (signal[index + 1] + signal[index - 1]);

    for (index = 1; index < signal_length - 2; index += 2)
        signal[index] += GAMMA * (signal[index - 1] + signal[index + 1]);

    signal[signal_length - 1] += 2.0 * GAMMA * signal[signal_length - 2];
    signal[0] = EPSILON * (signal[0] + 2.0 * DELTA * signal[1]);

    for (index = 2; index < signal_length; index += 2)
        signal[index] = EPSILON * (signal[index] + DELTA * (signal[index + 1] + signal[index - 1]));

    for (index = 1; index < signal_length; index += 2)
        signal[index] *= -INV_EPSILON;
}

void speck::CDF97::QccWAVCDF97SynthesisSymmetricEvenEven(double* signal,
                                                         size_t  signal_length)
{
    size_t index;

    for (index = 1; index < signal_length; index += 2)
        signal[index] *= (-EPSILON);

    signal[0] = signal[0] * INV_EPSILON - 2.0 * DELTA * signal[1];

    for (index = 2; index < signal_length; index += 2)
        signal[index] = signal[index] * INV_EPSILON - DELTA * (signal[index + 1] + signal[index - 1]);

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

void speck::CDF97::QccWAVCDF97SynthesisSymmetricOddEven(double* signal,
                                                        size_t  signal_length)
{
    size_t index;

    for (index = 1; index < signal_length - 1; index += 2)
        signal[index] *= (-EPSILON);

    signal[0] = signal[0] * INV_EPSILON - 2.0 * DELTA * signal[1];

    for (index = 2; index < signal_length - 2; index += 2)
        signal[index] = signal[index] * INV_EPSILON - DELTA * (signal[index + 1] + signal[index - 1]);

    signal[signal_length - 1] = signal[signal_length - 1] * INV_EPSILON - 2.0 * DELTA * signal[signal_length - 2];

    for (index = 1; index < signal_length - 1; index += 2)
        signal[index] -= GAMMA * (signal[index - 1] + signal[index + 1]);

    signal[0] -= 2.0 * BETA * signal[1];

    for (index = 2; index < signal_length - 2; index += 2)
        signal[index] -= BETA * (signal[index + 1] + signal[index - 1]);

    signal[signal_length - 1] -= 2.0 * BETA * signal[signal_length - 2];

    for (index = 1; index < signal_length - 1; index += 2)
        signal[index] -= ALPHA * (signal[index - 1] + signal[index + 1]);
}

void speck::CDF97::QccWAVCDF97AnalysisSymmetricOddEven(double* signal,
                                                       size_t  signal_length)
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
        signal[index] = EPSILON * (signal[index] + DELTA * (signal[index + 1] + signal[index - 1]));

    signal[signal_length - 1] = EPSILON * (signal[signal_length - 1] + 2 * DELTA * signal[signal_length - 2]);

    for (index = 1; index < signal_length - 1; index += 2)
        signal[index] *= (-INV_EPSILON);
}

void speck::CDF97::set_mean(double m)
{
    m_data_mean = m;
}

void speck::CDF97::set_dims(size_t x, size_t y, size_t z)
{
    assert(m_buf_len == x * y * z || m_buf_len == 0);

    m_dim_x   = x;
    m_dim_y   = y;
    m_dim_z   = z;
    m_buf_len = x * y * z;
}

auto speck::CDF97::get_mean() const -> double
{
    return m_data_mean;
}

void speck::CDF97::get_dims(size_t& x, size_t& y) const
{
    x = m_dim_x;
    y = m_dim_y;
}

void speck::CDF97::get_dims(size_t& x, size_t& y, size_t& z) const
{
    x = m_dim_x;
    y = m_dim_y;
    z = m_dim_z;
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
