#include "CDF97.h"

#include <algorithm>
#include <cassert>
#include <cstring> // std::memcpy()
#include <numeric> // std::accumulate()
#include <type_traits>


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
    if( m_col_buf_len < max_col * 2 ) {
        m_col_buf_len = max_col * 2;
        m_col_buf     = std::make_unique<double[]>(m_col_buf_len);
    }

    auto max_slice = std::max( std::max(dims[0] * dims[1], dims[0] * dims[2]), dims[1] * dims[2] );
    if( m_slice_buf_len < max_slice ) {
        m_slice_buf_len = max_slice;
        m_slice_buf     = std::make_unique<double[]>(m_slice_buf_len);
    }

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
    if( m_col_buf_len < max_col * 2 ) {
        m_col_buf_len = max_col * 2;
        m_col_buf     = std::make_unique<double[]>(m_col_buf_len);
    }

    auto max_slice = std::max( std::max(dims[0] * dims[1], dims[0] * dims[2]), dims[1] * dims[2] );
    if( m_slice_buf_len < max_slice ) {
        m_slice_buf_len = max_slice;
        m_slice_buf     = std::make_unique<double[]>(m_slice_buf_len);
    }

    return RTNType::Good;
}

auto speck::CDF97::view_data() const -> const vecd_type&
{
    return m_data_buf;
}

auto speck::CDF97::release_data() -> vecd_type&&
{
    m_dims = {0, 0, 0};
    return std::move(m_data_buf);
}

void speck::CDF97::dwt1d()
{
    size_t num_xforms = speck::num_of_xforms(m_dims[0]);
    m_dwt1d(m_data_buf.data(), m_data_buf.size(), num_xforms);
}

void speck::CDF97::idwt1d()
{
    size_t num_xforms = speck::num_of_xforms(m_dims[0]);
    m_idwt1d(m_data_buf.data(), m_data_buf.size(), num_xforms);
}

void speck::CDF97::dwt2d()
{
    size_t num_xforms_xy = speck::num_of_xforms(std::min(m_dims[0], m_dims[1]));
    m_dwt2d(m_data_buf.data(), {m_dims[0], m_dims[1]}, num_xforms_xy);
}

void speck::CDF97::idwt2d()
{
    size_t num_xforms_xy = speck::num_of_xforms(std::min(m_dims[0], m_dims[1]));
    m_idwt2d(m_data_buf.data(), {m_dims[0], m_dims[1]}, num_xforms_xy);
}

//void speck::CDF97::dwt3d_wavelet_packet()
void speck::CDF97::dwt3d()
{
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

    const size_t  plane_size_xy = m_dims[0] * m_dims[1];

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
            m_dwt1d(m_slice_buf.get() + x * m_dims[2], m_dims[2], num_xforms_z);

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
        m_dwt2d(m_data_buf.data() + offset, {m_dims[0], m_dims[1]}, num_xforms_xy);
    }
}

//void speck::CDF97::idwt3d_wavelet_packet()
void speck::CDF97::idwt3d()
{
    const size_t plane_size_xy = m_dims[0] * m_dims[1];

    // First, inverse transform each plane
    //
    auto num_xforms_xy = speck::num_of_xforms(std::min(m_dims[0], m_dims[1]));

    for (size_t i = 0; i < m_dims[2]; i++) {
        const size_t offset  = plane_size_xy * i;
        m_idwt2d(m_data_buf.data() + offset, {m_dims[0], m_dims[1]}, num_xforms_xy);
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
            m_idwt1d(m_slice_buf.get() + x * m_dims[2], m_dims[2], num_xforms_z);

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

void speck::CDF97::m_dwt2d(double*                plane,
                           std::array<size_t, 2>  len_xy,
                           size_t                 num_of_lev)
{
    for (size_t lev = 0; lev < num_of_lev; lev++) {
        auto approx_x = speck::calc_approx_detail_len(len_xy[0], lev);
        auto approx_y = speck::calc_approx_detail_len(len_xy[1], lev);
        m_dwt2d_one_level(plane, {approx_x[0], approx_y[0]});
    }
}

void speck::CDF97::m_idwt2d(double*               plane,
                            std::array<size_t, 2> len_xy,
                            size_t                num_of_lev)
{
    for (size_t lev = num_of_lev; lev > 0; lev--) {
        auto approx_x = speck::calc_approx_detail_len(len_xy[0], lev - 1);
        auto approx_y = speck::calc_approx_detail_len(len_xy[1], lev - 1);
        m_idwt2d_one_level(plane, {approx_x[0], approx_y[0]});
    }
}

void speck::CDF97::m_dwt1d(double* array,
                           size_t  array_len,
                           size_t  num_of_lev)
{
    for (size_t lev = 0; lev < num_of_lev; lev++) {
        auto [apx, nnm] = speck::calc_approx_detail_len(array_len, lev);
        m_dwt1d_one_level(array, apx);
    }
}

void speck::CDF97::m_idwt1d(double* array,
                            size_t  array_len,
                            size_t  num_of_lev)
{
    for (size_t lev = num_of_lev; lev > 0; lev--) {
        auto [apx, nnm] = speck::calc_approx_detail_len(array_len, lev - 1);
        m_idwt1d_one_level(array, apx);
    }
}

void speck::CDF97::m_dwt1d_one_level(double* array,
                                     size_t  array_len)
{
    std::memcpy(m_col_buf.get(), array, sizeof(double) * array_len);
    if (array_len % 2 == 0) [[likely]] {
        this->QccWAVCDF97AnalysisSymmetricEvenEven(m_col_buf.get(), array_len);
        m_gather_even(array, m_col_buf.get(), array_len);
    } else {
        this->QccWAVCDF97AnalysisSymmetricOddEven(m_col_buf.get(), array_len);
        m_gather_odd(array, m_col_buf.get(), array_len);
    }
}

void speck::CDF97::m_idwt1d_one_level(double* array,
                                      size_t  array_len)
{
    if (array_len % 2 == 0) [[likely]] {
        m_scatter_even(m_col_buf.get(), array, array_len);
        this->QccWAVCDF97SynthesisSymmetricEvenEven(m_col_buf.get(), array_len);
    } else {
        m_scatter_odd(m_col_buf.get(), array, array_len);
        this->QccWAVCDF97SynthesisSymmetricOddEven(m_col_buf.get(), array_len);
    }
    std::memcpy(array, m_col_buf.get(), sizeof(double) * array_len);
}

void speck::CDF97::m_dwt2d_one_level(double*                plane,
                                     std::array<size_t, 2>  len_xy)
{
    // Specify temporary buffers to work on
    size_t        max_len  = std::max(len_xy[0], len_xy[1]);
    double* const buf_ptr  = m_col_buf.get();   // First half of the array
    double* const buf_ptr2 = buf_ptr + max_len; // Second half of the array

    // First, perform DWT along X for every row
    if (len_xy[0] % 2 == 0) [[likely]] // Even length
    {
        for (size_t i = 0; i < len_xy[1]; i++) {
            auto* pos = plane + i * m_dims[0];
            std::memcpy(buf_ptr, pos, sizeof(double) * len_xy[0]);
            this->QccWAVCDF97AnalysisSymmetricEvenEven(buf_ptr, len_xy[0]);
            // pub back the resluts in low-pass and high-pass groups
            m_gather_even(pos, buf_ptr, len_xy[0]);
        }
    } else // Odd length
    {
        for (size_t i = 0; i < len_xy[1]; i++) {
            auto* pos = plane + i * m_dims[0];
            std::memcpy(buf_ptr, pos, sizeof(double) * len_xy[0]);
            this->QccWAVCDF97AnalysisSymmetricOddEven(buf_ptr, len_xy[0]);
            // pub back the resluts in low-pass and high-pass groups
            m_gather_odd(pos, buf_ptr, len_xy[0]);
        }
    }

    // Second, perform DWT along Y for every column
    // Note, I've tested that up to 1024^2 planes it is actually slightly slower
    // to transpose the plane and then perform the transforms. This was consistent
    // on both a MacBook and a RaspberryPi 3. Note2, I've tested transpose again
    // on an X86 linux machine using gcc, clang, and pgi. Again the difference is
    // either indistinguishable, or the current implementation has a slight edge.

    if (len_xy[1] % 2 == 0) [[likely]] // Even length
    {
        for (size_t x = 0; x < len_xy[0]; x++) {
            for (size_t y = 0; y < len_xy[1]; y++)
                buf_ptr[y] = plane[y * m_dims[0] + x];
            this->QccWAVCDF97AnalysisSymmetricEvenEven(buf_ptr, len_xy[1]);
            // Re-organize the resluts in low-pass and high-pass groups
            m_gather_even(buf_ptr2, buf_ptr, len_xy[1]);
            for (size_t y = 0; y < len_xy[1]; y++)
                plane[y * m_dims[0] + x] = buf_ptr2[y];
        }
    } else // Odd length
    {
        for (size_t x = 0; x < len_xy[0]; x++) {
            for (size_t y = 0; y < len_xy[1]; y++)
                buf_ptr[y] = plane[y * m_dims[0] + x];
            this->QccWAVCDF97AnalysisSymmetricOddEven(buf_ptr, len_xy[1]);
            // Re-organize the resluts in low-pass and high-pass groups
            m_gather_odd(buf_ptr2, buf_ptr, len_xy[1]);
            for (size_t y = 0; y < len_xy[1]; y++)
                plane[y * m_dims[0] + x] = buf_ptr2[y];
        }
    }
}

void speck::CDF97::m_idwt2d_one_level(double*               plane,
                                      std::array<size_t, 2> len_xy)
{
    // Specify temporary buffers to work on
    size_t        max_len  = std::max(len_xy[0], len_xy[1]);
    double* const buf_ptr  = m_col_buf.get();   // First half of the array
    double* const buf_ptr2 = buf_ptr + max_len; // Second half of the array

    // First, perform IDWT along Y for every column
    if (len_xy[1] % 2 == 0) [[likely]] // Even length
    {
        for (size_t x = 0; x < len_xy[0]; x++) {
            for (size_t y = 0; y < len_xy[1]; y++)
                buf_ptr[y] = plane[y * m_dims[0] + x];
            // Re-organize the coefficients as interleaved low-pass and high-pass ones
            m_scatter_even(buf_ptr2, buf_ptr, len_xy[1]);
            this->QccWAVCDF97SynthesisSymmetricEvenEven(buf_ptr2, len_xy[1]);
            for (size_t y = 0; y < len_xy[1]; y++)
                plane[y * m_dims[0] + x] = buf_ptr2[y];
        }
    } else // Odd length
    {
        for (size_t x = 0; x < len_xy[0]; x++) {
            for (size_t y = 0; y < len_xy[1]; y++)
                buf_ptr[y] = plane[y * m_dims[0] + x];
            // Re-organize the coefficients as interleaved low-pass and high-pass ones
            m_scatter_odd(buf_ptr2, buf_ptr, len_xy[1]);
            this->QccWAVCDF97SynthesisSymmetricOddEven(buf_ptr2, len_xy[1]);
            for (size_t y = 0; y < len_xy[1]; y++)
                plane[y * m_dims[0] + x] = buf_ptr2[y];
        }
    }

    // Second, perform IDWT along X for every row
    if (len_xy[0] % 2 == 0) [[likely]] // Even length
    {
        for (size_t i = 0; i < len_xy[1]; i++) {
            auto* pos = plane + i * m_dims[0];
            // Re-organize the coefficients as interleaved low-pass and high-pass ones
            m_scatter_even(buf_ptr, pos, len_xy[0]);
            this->QccWAVCDF97SynthesisSymmetricEvenEven(buf_ptr, len_xy[0]);
            std::memcpy(pos, buf_ptr, sizeof(double) * len_xy[0]);
        }
    } else // Odd length
    {
        for (size_t i = 0; i < len_xy[1]; i++) {
            auto* pos = plane + i * m_dims[0];
            // Re-organize the coefficients as interleaved low-pass and high-pass ones
            m_scatter_odd(buf_ptr, pos, len_xy[0]);
            this->QccWAVCDF97SynthesisSymmetricOddEven(buf_ptr, len_xy[0]);
            std::memcpy(pos, buf_ptr, sizeof(double) * len_xy[0]);
        }
    }
}


void speck::CDF97::m_dwt3d_one_level(double*                plane,
                                     std::array<size_t, 3>  len_xyz)
{
    // First do one level of transform on all XY planes.
    const size_t plane_size_xy = m_dims[0] * m_dims[1];
    for (size_t z = 0; z < len_xyz[2]; z++) {
        const size_t offset = plane_size_xy * z;
        m_dwt2d_one_level(plane + offset, {len_xyz[0], len_xyz[1]});
    }

    // Second do one level of transform on all Z columns.
}


void speck::CDF97::m_gather_even(double* dest, const double* orig, size_t len) const
{
    assert(len % 2 == 0); // This function specifically for even length input
    size_t low_count = len / 2, high_count = len / 2;
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

