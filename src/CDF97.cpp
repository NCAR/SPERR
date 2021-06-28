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
auto speck::CDF97::copy_data(const T* data, size_t len, dims_type dims ) -> RTNType
{
    static_assert(std::is_floating_point<T>::value,
                  "!! Only floating point values are supported !!");
    if( len != dims[0] * dims[1] * dims[2] )
        return RTNType::WrongSize;

    m_data_buf.resize( len );
    std::copy( data, data + len, m_data_buf.begin() );

    m_dims = dims;

    auto max_col = std::max( std::max(dims[0], dims[1]), dims[2] );
    m_col_buf.resize( max_col * 2 );

    auto max_slice = std::max( std::max(dims[0] * dims[1], dims[0] * dims[2]), dims[1] * dims[2] );
    m_slice_buf.resize( max_slice );

    return RTNType::Good;
}
template auto speck::CDF97::copy_data(const float*,  size_t, dims_type ) -> RTNType;
template auto speck::CDF97::copy_data(const double*, size_t, dims_type ) -> RTNType;


auto speck::CDF97::take_data( vecd_type&& buf, dims_type dims ) -> RTNType
{
    if( buf.size() != dims[0] * dims[1] * dims[2] )
        return RTNType::WrongSize;

    m_data_buf = std::move(buf);
    m_dims     = dims;

    auto max_col = std::max( std::max(dims[0], dims[1]), dims[2] );
    m_col_buf.resize( max_col * 2 );

    auto max_slice = std::max( std::max(dims[0] * dims[1], dims[0] * dims[2]), dims[1] * dims[2] );
    m_slice_buf.resize( max_slice );

    return RTNType::Good;
}

auto speck::CDF97::view_data() const -> const vecd_type&
{
    return m_data_buf;
}

auto speck::CDF97::release_data() -> vecd_type
{
    m_dims = {0, 0, 0};
    return std::move(m_data_buf);
}

void speck::CDF97::dwt1d()
{
    size_t num_xforms = speck::num_of_xforms(m_dims[0]);
    m_dwt1d(m_data_buf.data(), m_data_buf.size(), num_xforms, m_col_buf.data());
}

void speck::CDF97::idwt1d()
{
    size_t num_xforms = speck::num_of_xforms(m_dims[0]);
    m_idwt1d(m_data_buf.data(), m_data_buf.size(), num_xforms, m_col_buf.data());
}

void speck::CDF97::dwt2d()
{
    size_t num_xforms_xy = speck::num_of_xforms(std::min(m_dims[0], m_dims[1]));
    m_dwt2d(m_data_buf.data(), m_dims[0], m_dims[1], num_xforms_xy, m_col_buf.data());
}

void speck::CDF97::idwt2d()
{
    size_t num_xforms_xy = speck::num_of_xforms(std::min(m_dims[0], m_dims[1]));
    m_idwt2d(m_data_buf.data(), m_dims[0], m_dims[1], num_xforms_xy, m_col_buf.data());
}

void speck::CDF97::dwt3d()
{
    const size_t  plane_size_xy = m_dims[0] * m_dims[1];

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
    const auto num_xforms_z = speck::num_of_xforms(m_dims[2]);

    for (size_t y = 0; y < m_dims[1]; y++) {
        const auto y_offset = y * m_dims[0];

        // Re-arrange values of one XZ slice so that they form many z_columns
        for (size_t z = 0; z < m_dims[2]; z++) {
            const auto cube_start_idx = z * plane_size_xy + y_offset;
            for (size_t x = 0; x < m_dims[0]; x++)
                m_slice_buf[z + x * m_dims[2]] = m_data_buf[cube_start_idx + x];
        }

        // DWT1D on every z_column
        for (size_t x = 0; x < m_dims[0]; x++)
            m_dwt1d(m_slice_buf.data() + x * m_dims[2], m_dims[2], num_xforms_z, m_col_buf.data());

        // Put back values of the z_columns to the cube
        for (size_t z = 0; z < m_dims[2]; z++) {
            const auto cube_start_idx = z * plane_size_xy + y_offset;
            for (size_t x = 0; x < m_dims[0]; x++)
                m_data_buf[cube_start_idx + x] = m_slice_buf[z + x * m_dims[2]];
        }
    }

    // Second transform each plane
    //
    const auto num_xforms_xy = speck::num_of_xforms(std::min(m_dims[0], m_dims[1]));

    for (size_t z = 0; z < m_dims[2]; z++) {
        const size_t offset = plane_size_xy * z;
        m_dwt2d(m_data_buf.data() + offset, m_dims[0], m_dims[1], num_xforms_xy, m_col_buf.data());
    }
}

void speck::CDF97::idwt3d()
{
    const size_t plane_size_xy = m_dims[0] * m_dims[1];

    // First, inverse transform each plane
    //
    auto num_xforms_xy = speck::num_of_xforms(std::min(m_dims[0], m_dims[1]));

    for (size_t i = 0; i < m_dims[2]; i++) {
        const size_t offset  = plane_size_xy * i;
        m_idwt2d(m_data_buf.data() + offset, m_dims[0], m_dims[1], num_xforms_xy, m_col_buf.data());
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
    const auto num_xforms_z = speck::num_of_xforms(m_dims[2]);

    for (size_t y = 0; y < m_dims[1]; y++) {
        const auto y_offset  = y * m_dims[0];

        // Re-arrange values on one slice so that they form many z_columns
        for (size_t z = 0; z < m_dims[2]; z++) {
            const auto cube_start_idx = z * plane_size_xy + y_offset;
            for (size_t x = 0; x < m_dims[0]; x++)
                m_slice_buf[z + x * m_dims[2]] = m_data_buf[cube_start_idx + x];
        }

        // IDWT1D on every z_column
        for (size_t x = 0; x < m_dims[0]; x++)
            m_idwt1d(m_slice_buf.data() + x * m_dims[2], m_dims[2], num_xforms_z, m_col_buf.data());

        // Put back values from the z_columns to the cube
        for (size_t z = 0; z < m_dims[2]; z++) {
            const auto cube_start_idx = z * plane_size_xy + y_offset;
            for (size_t x = 0; x < m_dims[0]; x++)
                m_data_buf[cube_start_idx + x] = m_slice_buf[z + x * m_dims[2]];
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
            auto* pos = plane + i * m_dims[0];
            std::memcpy(buf_ptr, pos, sizeof(double) * len_x);
            this->QccWAVCDF97AnalysisSymmetricEvenEven(buf_ptr, len_x);
            // pub back the resluts in low-pass and high-pass groups
            m_gather_even(pos, buf_ptr, len_x);
        }
    } else // Odd length
    {
        for (size_t i = 0; i < len_y; i++) {
            auto* pos = plane + i * m_dims[0];
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
                buf_ptr[y] = plane[y * m_dims[0] + x];
            this->QccWAVCDF97AnalysisSymmetricEvenEven(buf_ptr, len_y);
            // Re-organize the resluts in low-pass and high-pass groups
            m_gather_even(buf_ptr2, buf_ptr, len_y);
            for (size_t y = 0; y < len_y; y++)
                plane[y * m_dims[0] + x] = buf_ptr2[y];
        }
    } else // Odd length
    {
        for (size_t x = 0; x < len_x; x++) {
            for (size_t y = 0; y < len_y; y++)
                buf_ptr[y] = plane[y * m_dims[0] + x];
            this->QccWAVCDF97AnalysisSymmetricOddEven(buf_ptr, len_y);
            // Re-organize the resluts in low-pass and high-pass groups
            m_gather_odd(buf_ptr2, buf_ptr, len_y);
            for (size_t y = 0; y < len_y; y++)
                plane[y * m_dims[0] + x] = buf_ptr2[y];
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
                buf_ptr[y] = plane[y * m_dims[0] + x];
            // Re-organize the coefficients as interleaved low-pass and high-pass ones
            m_scatter_even(buf_ptr2, buf_ptr, len_y);
            this->QccWAVCDF97SynthesisSymmetricEvenEven(buf_ptr2, len_y);
            for (size_t y = 0; y < len_y; y++)
                plane[y * m_dims[0] + x] = buf_ptr2[y];
        }
    } else // Odd length
    {
        for (size_t x = 0; x < len_x; x++) {
            for (size_t y = 0; y < len_y; y++)
                buf_ptr[y] = plane[y * m_dims[0] + x];
            // Re-organize the coefficients as interleaved low-pass and high-pass ones
            m_scatter_odd(buf_ptr2, buf_ptr, len_y);
            this->QccWAVCDF97SynthesisSymmetricOddEven(buf_ptr2, len_y);
            for (size_t y = 0; y < len_y; y++)
                plane[y * m_dims[0] + x] = buf_ptr2[y];
        }
    }

    // Second, perform IDWT along X for every row
    if (len_x % 2 == 0) // Even length
    {
        for (size_t i = 0; i < len_y; i++) {
            auto* pos = plane + i * m_dims[0];
            // Re-organize the coefficients as interleaved low-pass and high-pass ones
            m_scatter_even(buf_ptr, pos, len_x);
            this->QccWAVCDF97SynthesisSymmetricEvenEven(buf_ptr, len_x);
            std::memcpy(pos, buf_ptr, sizeof(double) * len_x);
        }
    } else // Odd length
    {
        for (size_t i = 0; i < len_y; i++) {
            auto* pos = plane + i * m_dims[0];
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
    return m_dims;
}

