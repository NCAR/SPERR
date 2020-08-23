#include "CDF97.h"
#include "speck_helper.h"

#include <cassert>
#include <cstring> // for std::memcpy()
#include <type_traits>

template <typename T>
void speck::CDF97::copy_data(const T* data, size_t len)
{
    static_assert(std::is_floating_point<T>::value,
                  "!! Only floating point values are supported !!");

    assert(m_dim_x * m_dim_y * m_dim_z == 0 || m_dim_x * m_dim_y * m_dim_z == len);
    m_buf_len = len;
    m_data_buf = speck::unique_malloc<double>( len );
    for (size_t i = 0; i < len; i++)
        m_data_buf[i] = data[i];
}
template void speck::CDF97::copy_data(const float*, size_t);
template void speck::CDF97::copy_data(const double*, size_t);


void speck::CDF97::take_data(buffer_type_d ptr, size_t len)
{
    assert(m_dim_x * m_dim_y * m_dim_z == 0 || m_dim_x * m_dim_y * m_dim_z == len);
    m_buf_len  = len;
    m_data_buf = std::move(ptr);
}

auto speck::CDF97::get_read_only_data() const -> const buffer_type_d&
{
    return m_data_buf;
}

auto speck::CDF97::release_data() -> buffer_type_d
{
    return std::move(m_data_buf);
}

void speck::CDF97::dwt1d()
{
    m_calc_mean();
    for (size_t i = 0; i < m_buf_len; i++)
        m_data_buf[i] -= m_data_mean;

    size_t num_xforms = speck::num_of_xforms(m_dim_x);
    m_dwt1d(m_data_buf.get(), m_buf_len, num_xforms);
}

void speck::CDF97::idwt1d()
{
    size_t num_xforms = speck::num_of_xforms(m_dim_x);
    m_idwt1d(m_data_buf.get(), m_buf_len, num_xforms);

    for (size_t i = 0; i < m_buf_len; i++)
        m_data_buf[i] += m_data_mean;
}

void speck::CDF97::dwt2d()
{
    m_calc_mean();
    for (size_t i = 0; i < m_buf_len; i++)
        m_data_buf[i] -= m_data_mean;

    size_t num_xforms_xy = speck::num_of_xforms(std::min(m_dim_x, m_dim_y));
    m_dwt2d(m_data_buf.get(), m_dim_x, m_dim_y, num_xforms_xy);
}

void speck::CDF97::idwt2d()
{
    size_t num_xforms_xy = speck::num_of_xforms(std::min(m_dim_x, m_dim_y));
    m_idwt2d(m_data_buf.get(), m_dim_x, m_dim_y, num_xforms_xy);

    for (size_t i = 0; i < m_buf_len; i++)
        m_data_buf[i] += m_data_mean;
}

void speck::CDF97::dwt3d()
{
    m_calc_mean();
    for (size_t i = 0; i < m_buf_len; i++)
        m_data_buf[i] -= m_data_mean;

    size_t max_dim             = std::max(m_dim_x, m_dim_y);
    max_dim                    = std::max(max_dim, m_dim_z);
    buffer_type_d tmp_buf      = speck::unique_malloc<double>( max_dim * 2 );
    const size_t plane_size_xy = m_dim_x * m_dim_y;

    /*
     * Note on the order of performing transforms in 3 dimensions:
     * this implementation follows QccPack's example.
     *
     * First transform along the Z dimension
     */

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

    // Process one XZ slice at a time
    buffer_type_d z_columns = speck::unique_malloc<double>(m_dim_x * m_dim_z);
    const auto num_xforms_z = speck::num_of_xforms(m_dim_z);
    for (size_t y = 0; y < m_dim_y; y++) {
        const auto y_offset = y * m_dim_x;

        // Re-arrange values of one XZ slice so that they form many z_columns
        for (size_t z = 0; z < m_dim_z; z++) {
            const auto cube_start_idx = z * plane_size_xy + y_offset;
            for (size_t x = 0; x < m_dim_x; x++)
                z_columns[z + x * m_dim_z] = m_data_buf[cube_start_idx + x];
        }

        // DWT1D on every z_column
        for (size_t x = 0; x < m_dim_x; x++)
            m_dwt1d(z_columns.get() + x * m_dim_z, m_dim_z, num_xforms_z, tmp_buf.get());

        // Put back values of the many z_columns to the cube
        for (size_t z = 0; z < m_dim_z; z++) {
            const auto cube_start_idx = z * plane_size_xy + y_offset;
            for (size_t x = 0; x < m_dim_x; x++)
                m_data_buf[cube_start_idx + x] = z_columns[z + x * m_dim_z];
        }
    }

    // Second transform each plane
    const auto num_xforms_xy = speck::num_of_xforms(std::min(m_dim_x, m_dim_y));
    for (size_t i = 0; i < m_dim_z; i++) {
        size_t offset = plane_size_xy * i;
        m_dwt2d(m_data_buf.get() + offset, m_dim_x, m_dim_y, num_xforms_xy, tmp_buf.get());
    }
}

void speck::CDF97::idwt3d()
{
    size_t max_dim             = std::max(m_dim_x, m_dim_y);
    max_dim                    = std::max(max_dim, m_dim_z);
    buffer_type_d tmp_buf      = speck::unique_malloc<double>(max_dim * 2);
    const size_t plane_size_xy = m_dim_x * m_dim_y;

    // First, inverse transform each plane
    auto num_xforms_xy = speck::num_of_xforms(std::min(m_dim_x, m_dim_y));
    for (size_t i = 0; i < m_dim_z; i++) {
        size_t offset = plane_size_xy * i;
        m_idwt2d(m_data_buf.get() + offset, m_dim_x, m_dim_y, num_xforms_xy, tmp_buf.get());
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
    buffer_type_d z_columns = speck::unique_malloc<double>(m_dim_x * m_dim_z);
    const auto num_xforms_z = speck::num_of_xforms(m_dim_z);
    for (size_t y = 0; y < m_dim_y; y++) {
        const auto y_offset = y * m_dim_x;

        // Re-arrange values on one slice so that they form many z_columns
        for (size_t z = 0; z < m_dim_z; z++) {
            const auto cube_start_idx = z * plane_size_xy + y_offset;
            for (size_t x = 0; x < m_dim_x; x++)
                z_columns[z + x * m_dim_z] = m_data_buf[cube_start_idx + x];
        }

        // IDWT1D on every z_column
        for (size_t x = 0; x < m_dim_x; x++)
            m_idwt1d(z_columns.get() + x * m_dim_z, m_dim_z, num_xforms_z, tmp_buf.get());

        // Put back values from the many z_clumns to the cube
        for (size_t z = 0; z < m_dim_z; z++) {
            const auto cube_start_idx = z * plane_size_xy + y_offset;
            for (size_t x = 0; x < m_dim_x; x++)
                m_data_buf[cube_start_idx + x] = z_columns[z + x * m_dim_z];
        }
    }

    // Finally, add back the mean which was subtracted earlier.
    for (size_t i = 0; i < m_buf_len; i++)
        m_data_buf[i] += m_data_mean;
}

//
// Private Methods
//
void speck::CDF97::m_calc_mean()
{
    assert(m_dim_x > 0 && m_dim_y > 0 && m_dim_z > 0);

    /*
     * Here we calculate mean row by row to avoid too big numbers.
     *   Not using Kahan summation because that's hard to vectorize.
     *   Also, one test shows that this implementation is 4X faster than Kahan.
     */
    buffer_type_d row_means = speck::unique_malloc<double>(m_dim_y * m_dim_z);
    const double dim_x1     = 1.0 / double(m_dim_x);
    size_t       counter1   = 0, counter2 = 0;
    for (size_t z = 0; z < m_dim_z; z++) {
        for (size_t y = 0; y < m_dim_y; y++) {
            double sum = 0.0;
            for (size_t x = 0; x < m_dim_x; x++) {
                sum += m_data_buf[counter1++];
            }
            row_means[counter2++] = sum * dim_x1;
        }
    }

    buffer_type_d layer_means = speck::unique_malloc<double>(m_dim_z);
    const double dim_y1       = 1.0 / double(m_dim_y);
    counter1                  = 0;
    counter2                  = 0;
    for (size_t z = 0; z < m_dim_z; z++) {
        double sum = 0.0;
        for (size_t y = 0; y < m_dim_y; y++) {
            sum += row_means[counter1++];
        }
        layer_means[counter2++] = sum * dim_y1;
    }

    double sum = 0.0;
    for (size_t z = 0; z < m_dim_z; z++)
        sum += layer_means[z];

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
    }
    else {
        buf = speck::unique_malloc<double>(array_len);
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
    }
    else {
        buf = speck::unique_malloc<double>(array_len);
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
    } 
    else {
        buffer   = speck::unique_malloc<double>(len_xy * 2);
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
    } 
    else {
        buffer   = speck::unique_malloc<double>(len_xy * 2);
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

void speck::CDF97::get_dims(std::array<size_t, 2>& dims) const
{
    dims[0] = m_dim_x;
    dims[1] = m_dim_y;
}

void speck::CDF97::get_dims(std::array<size_t, 3>& dims) const
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
