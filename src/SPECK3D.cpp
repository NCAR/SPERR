#include "SPECK3D.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>

//
// Class SPECK3D
//
void speck::SPECK3D::set_dims(size_t x, size_t y, size_t z)
{
    // Sanity Check
    assert(m_coeff_len == 0 || m_coeff_len == x * y * z);
    m_dim_x     = x;
    m_dim_y     = y;
    m_dim_z     = z;
    m_coeff_len = x * y * z;
}

void speck::SPECK3D::get_dims(size_t& x, size_t& y, size_t& z) const
{
    x = m_dim_x;
    y = m_dim_y;
    z = m_dim_z;
}

void speck::SPECK3D::set_max_coeff_bits(int32_t bits)
{
    m_max_coeff_bits = bits;
}

void speck::SPECK3D::set_bit_budget(size_t budget)
{
    size_t mod = budget % 8;
    if (mod == 0)
        m_budget = budget;
    else // we can fill up the last byte
        m_budget = budget + 8 - mod;
}

void speck::SPECK3D::m_clean_LIS()
{
    std::vector<SPECKSet3D> tmpv;

    for (size_t tmpi = 1; tmpi <= m_LIS_garbage_cnt.size(); tmpi++) {
        // Because lists towards the end tend to have bigger sizes, we look at
        // them first. This practices should reduce the number of memory allocations.
        const auto i = m_LIS_garbage_cnt.size() - tmpi;

        // Only consolidate memory if the garbage count is more than half
        if (m_LIS_garbage_cnt[i] > m_LIS[i].size() / 2) {
            auto& list = m_LIS[i];
            tmpv.clear();
            tmpv.reserve(list.size()); // will leave half capacity unfilled, so the list
                                       // won't need a memory re-allocation for a while.
            std::copy_if(list.cbegin(), list.cend(), std::back_inserter(tmpv),
                         [](const SPECKSet3D& s) { return s.type != SetType::Garbage; });
            std::swap(list, tmpv);
            m_LIS_garbage_cnt[i] = 0;
        }
    }
    // Re-claim memory held by tmpv
    tmpv.clear();
    tmpv.shrink_to_fit();

    // Since the last element of m_LIS is represented separately as m_LIP,
    //   let's also clean up that list.
    if (m_LIP_garbage_cnt > m_LIP.size() / 2) {
        std::vector<size_t> tmp_LIP;
        tmp_LIP.reserve(m_LIP.size());
        for (size_t i = 0; i < m_LIP.size(); i++) {
            if (!m_LIP_garbage[i])
                tmp_LIP.emplace_back(m_LIP[i]);
        }
        std::swap(m_LIP, tmp_LIP);
        m_LIP_garbage_cnt = 0;
        m_LIP_garbage.assign(m_LIP.size(), false);
    }
}

auto speck::SPECK3D::encode() -> int
{
    if( m_ready_to_encode() == false )
        return 1;
    m_encode_mode = true;

    m_initialize_sets_lists();

    m_bit_buffer.clear();
    m_bit_buffer.reserve(m_budget);
    auto max_coeff = speck::make_coeff_positive(m_coeff_buf, m_coeff_len, m_sign_array);

    // When max_coeff is between 0.0 and 1.0, std::log2(max_coeff) will become a
    // negative value. std::floor() will always find the smaller integer value,
    // which will always reconstruct to a bitplane value that is smaller than
    // max_coeff. Also, when max_coeff is close to 0.0, std::log2(max_coeff) can
    // have a pretty big magnitude, so we use int32_T here.
    m_max_coeff_bits = int32_t(std::floor(std::log2(max_coeff)));
    m_threshold      = std::pow(2.0, double(m_max_coeff_bits));
    for (size_t bitplane = 0; bitplane < 128; bitplane++) {
        // Update the significance map based on the current threshold
        // Most of them are gonna be false, and only a handful to be true.
        m_significance_map.assign(m_coeff_len, false);
        for (size_t i = 0; i < m_coeff_len; i++) {
            if (m_coeff_buf[i] >= m_threshold)
                m_significance_map[i] = true;
        }

        if (m_sorting_pass_encode())
            break;

        if (m_refinement_pass_encode())
            break;

        m_threshold *= 0.5;
        m_clean_LIS();
    }

    return 0;
}

auto speck::SPECK3D::decode() -> int
{
    if( m_ready_to_decode() == false )
        return 1;
    m_encode_mode = false;

    // By default, decode all the available bits
    if (m_budget == 0)
        m_budget = m_bit_buffer.size();
#ifdef NO_CPP14
    m_coeff_buf.reset(new double[m_coeff_len]);
#else
    m_coeff_buf = std::make_unique<double[]>(m_coeff_len);
#endif

    // initialize coefficients to be zero, and sign array to be all positive
    for (size_t i = 0; i < m_coeff_len; i++)
        m_coeff_buf[i] = 0.0;
    m_sign_array.assign(m_coeff_len, true);

    m_initialize_sets_lists();

    m_bit_idx   = 0;
    m_threshold = std::pow(2.0, float(m_max_coeff_bits));
    for (size_t bitplane = 0; bitplane < 128; bitplane++) {
        if (m_sorting_pass_decode())
            break;

        if (m_refinement_pass_decode())
            break;

        m_threshold *= 0.5;

        m_clean_LIS();
    }

    // If the loop above aborted before all newly significant pixels are initialized,
    // we finish them here!
    for (size_t i = 0; i < m_LSP_newly.size(); i++) {
        if (m_LSP_newly[i])
            m_coeff_buf[m_LSP[i]] = 1.5 * m_threshold;
    }

    // Restore coefficient signs by setting some of them negative
    for (size_t i = 0; i < m_sign_array.size(); i++) {
        if (!m_sign_array[i])
            m_coeff_buf[i] = -m_coeff_buf[i];
    }

    return 0;
}

void speck::SPECK3D::m_initialize_sets_lists()
{
    std::array<size_t, 3> num_of_parts; // how many times each dimension could be partitioned?
    num_of_parts[0] = speck::num_of_partitions( m_dim_x );
    num_of_parts[1] = speck::num_of_partitions( m_dim_y );
    num_of_parts[2] = speck::num_of_partitions( m_dim_z );
    size_t num_of_sizes = 1;
    for (size_t i = 0; i < 3; i++)
        num_of_sizes += num_of_parts[i];

    // initialize LIS
    m_LIS.clear();
    m_LIS.resize(num_of_sizes);
    m_LIS_garbage_cnt.assign(num_of_sizes, 0);
    m_LIP.clear();
    m_LIP_garbage.clear();
    m_LIP_garbage_cnt = 0;

    // Starting from a set representing the whole volume, identify the smaller sets
    //   and put them in LIS accordingly.
    SPECKSet3D big;
    big.length_x = uint32_t(m_dim_x); // Truncate 64-bit int to 32-bit, but should be OK.
    big.length_y = uint32_t(m_dim_y); // Truncate 64-bit int to 32-bit, but should be OK.
    big.length_z = uint32_t(m_dim_z); // Truncate 64-bit int to 32-bit, but should be OK.

    // clang-format off
    const auto num_of_xforms_xy = speck::num_of_xforms(std::min(m_dim_x, m_dim_y));
    const auto num_of_xforms_z  = speck::num_of_xforms(m_dim_z);
    size_t     xf               = 0;
    // clang-format on
    std::array<SPECKSet3D, 8> subsets;
    while (xf < num_of_xforms_xy && xf < num_of_xforms_z) {
        speck::partition_S_XYZ(big, subsets);
        big = subsets[0];
        for (size_t i = 1; i < 8; i++) {
            const auto parts = subsets[i].part_level;
            m_LIS[parts].emplace_back(subsets[i]);
        }
        xf++;
    }

    // One of these two conditions could happen if num_of_xforms_xy != num_of_xforms_z
    if (xf < num_of_xforms_xy) {
        std::array<SPECKSet3D, 4> sub4;
        while (xf < num_of_xforms_xy) {
            speck::partition_S_XY(big, sub4);
            big = sub4[0];
            for (size_t i = 1; i < 4; i++) {
                const auto parts = sub4[i].part_level;
                m_LIS[parts].emplace_back(sub4[i]);
            }
            xf++;
        }
    } else if (xf < num_of_xforms_z) {
        std::array<SPECKSet3D, 2> sub2;
        while (xf < num_of_xforms_z) {
            speck::partition_S_Z(big, sub2);
            big              = sub2[0];
            const auto parts = sub2[1].part_level;
            m_LIS[parts].emplace_back(sub2[1]);
            xf++;
        }
    }

    // Right now big is the set that's most likely to be significant, so insert
    // it at the front of it's corresponding vector. One-time expense.
    const auto parts = big.part_level;
#ifdef NO_CPP14
    m_LIS[parts].insert(m_LIS[parts].begin(), big);
#else
    m_LIS[parts].insert(m_LIS[parts].cbegin(), big);
#endif

    // initialize LSP
    m_LSP.clear();
    m_LSP_newly.clear();
}

auto speck::SPECK3D::m_sorting_pass_encode() -> int
{
#ifdef PRINT
    std::cout << "--> Sorting Pass " << std::endl;
#endif
    int rtn = 0;

    // Since we have a separate representation of LIP, let's process that list first!
    for (size_t i = 0; i < m_LIP.size(); i++) {
        if (!m_LIP_garbage[i]) {
            if ((rtn = m_process_P_encode(i)))
                return rtn;
        }
    }

    for (size_t tmp = 0; tmp < m_LIS.size(); tmp++) {
        // From the end of m_LIS to its front
        size_t idx1 = m_LIS.size() - 1 - tmp;
        for (size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++) {
            const auto& s = m_LIS[idx1][idx2];
            if (s.type != SetType::Garbage) {
                if ((rtn = m_process_S_encode(idx1, idx2)))
                    return rtn;
            }
        }
    }

    return 0;
}

auto speck::SPECK3D::m_sorting_pass_decode() -> int
{
    int rtn = 0;
    // Since we have a separate representation of LIP, let's process that list first!
    for (size_t i = 0; i < m_LIP.size(); i++) {
        if (!m_LIP_garbage[i]) {
            if ((rtn = m_process_P_decode(i)))
                return rtn;
        }
    }

    for (size_t tmp = 0; tmp < m_LIS.size(); tmp++) {
        // From the end of m_LIS to its front
        size_t idx1 = m_LIS.size() - 1 - tmp;
        for (size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++) {
            const auto& s = m_LIS[idx1][idx2];
            if (s.type != SetType::Garbage) {
                if ((rtn = m_process_S_decode(idx1, idx2)))
                    return rtn;
            }
        }
    }

    return 0;
}

auto speck::SPECK3D::m_refinement_pass_encode() -> int
{
    for (size_t i = 0; i < m_LSP.size(); i++) {
        const auto pos = m_LSP[i];
        if (m_LSP_newly[i]) // This is pixel is newly identified!
        {
            m_coeff_buf[pos] -= m_threshold;
            m_LSP_newly[i] = false;
        } else {
            if (m_coeff_buf[pos] >= m_threshold) {
                m_bit_buffer.push_back(true);
                m_coeff_buf[pos] -= m_threshold;
#ifdef PRINT
                std::cout << "r1" << std::endl;
#endif
            } else {
                m_bit_buffer.push_back(false);
#ifdef PRINT
                std::cout << "r0" << std::endl;
#endif
            }

            // Let's also see if we've reached the bit budget
            if (m_bit_buffer.size() >= m_budget)
                return 1;
        }
    }

    return 0;
}

auto speck::SPECK3D::m_refinement_pass_decode() -> int
{
    for (size_t i = 0; i < m_LSP.size(); i++) {
        const auto pos = m_LSP[i];
        if (m_LSP_newly[i]) {
            // Newly identified pixels are initialized.
            m_coeff_buf[pos] = 1.5 * m_threshold;
            m_LSP_newly[i]   = false;
        } else {
            if (m_bit_idx >= m_budget || m_bit_idx >= m_bit_buffer.size())
                return 1;

            m_coeff_buf[pos] += m_bit_buffer[m_bit_idx++] ? m_threshold * 0.5 : m_threshold * -0.5;
        }
    }

    return 0;
}

auto speck::SPECK3D::m_process_P_encode(size_t loc) -> int
{
    const auto pixel_idx = m_LIP[loc];

    // decide the significance of this pixel
    const bool this_pixel_is_sig = m_significance_map[pixel_idx];
    m_bit_buffer.push_back(this_pixel_is_sig);

#ifdef PRINT
    if (this_pixel_is_sig)
        std::cout << "s1" << std::endl;
    else
        std::cout << "s0" << std::endl;
#endif

    // When encoding, check bit budget after outputing a bit
    if (m_bit_buffer.size() >= m_budget)
        return 1;

    if (this_pixel_is_sig) {
        // Output pixel sign
        m_bit_buffer.push_back(m_sign_array[pixel_idx]);

#ifdef PRINT
        if (m_sign_array[pixel_idx])
            std::cout << "p1" << std::endl;
        else
            std::cout << "p0" << std::endl;
#endif
        // Note that after outputing two bits this pixel got put in LSP.
        // The same is reversed when decoding.
        m_LSP.push_back(pixel_idx);
        m_LSP_newly.push_back(true);

        m_LIP_garbage[loc] = true;
        m_LIP_garbage_cnt++;

        // When encoding, check bit budget after outputing a bit
        if (m_bit_buffer.size() >= m_budget)
            return 1;
    }

    return 0;
}

auto speck::SPECK3D::m_process_S_encode(size_t idx1, size_t idx2) -> int
{
    auto& set = m_LIS[idx1][idx2];
    assert(!set.is_pixel()); // helps debug
    int rtn = 0;

    // decide the significance of this set
    set.signif              = Significance::Insig;
    const size_t slice_size = m_dim_x * m_dim_y;
    for (auto z = set.start_z; z < (set.start_z + set.length_z); z++) {
        const size_t slice_offset = z * slice_size;
        for (auto y = set.start_y; y < (set.start_y + set.length_y); y++) {
            const size_t col_offset = slice_offset + y * m_dim_x;

            // Note: use std::any_of() isn't faster...
            for (auto x = set.start_x; x < (set.start_x + set.length_x); x++) {
                if (m_significance_map[col_offset + x]) {
                    set.signif = Significance::Sig;
                    goto end_loop_label;
                }
            }
        }
    }
end_loop_label:
    m_bit_buffer.push_back(set.signif == Significance::Sig); // output the significance value
    if (m_bit_buffer.size() >= m_budget)
        return 1;

#ifdef PRINT
    if (m_bit_buffer.back())
        std::cout << "s1" << std::endl;
    else
        std::cout << "s0" << std::endl;
#endif

    if (set.signif == Significance::Sig) {
        if ((rtn = m_code_S(idx1, idx2)))
            return rtn;

        set.type = SetType::Garbage; // this current set is gonna be discarded.
        m_LIS_garbage_cnt[set.part_level]++;
    }

    return 0;
}

auto speck::SPECK3D::m_process_P_decode(size_t loc) -> int
{
    // When decoding, check bit budget before attempting to read a bit
    if (m_bit_idx >= m_budget || m_bit_idx >= m_bit_buffer.size())
        return 1;
    const bool this_pixel_is_sig = m_bit_buffer[m_bit_idx++];

    if (this_pixel_is_sig) {
        const auto pixel_idx = m_LIP[loc];

        // When decoding, check bit budget before attempting to read a bit
        if (m_bit_idx >= m_budget || m_bit_idx >= m_bit_buffer.size())
            return 1;
        if (!m_bit_buffer[m_bit_idx++])
            m_sign_array[pixel_idx] = false;

        // Record to be initialized
        m_LSP.push_back(pixel_idx);
        m_LSP_newly.push_back(true);

        m_LIP_garbage[loc] = true;
        m_LIP_garbage_cnt++;
    }

    return 0;
}

auto speck::SPECK3D::m_process_S_decode(size_t idx1, size_t idx2) -> int
{
    assert(!m_LIS[idx1][idx2].is_pixel());

    if (m_bit_idx >= m_budget || m_bit_idx >= m_bit_buffer.size())
        return 1;

    auto& set  = m_LIS[idx1][idx2];
    set.signif = m_bit_buffer[m_bit_idx++] ? Significance::Sig : Significance::Insig;
    int rtn    = 0;

    if (set.signif == Significance::Sig) {
        if ((rtn = m_code_S(idx1, idx2)))
            return rtn;

        set.type = SetType::Garbage; // this current set is gonna be discarded.
        m_LIS_garbage_cnt[set.part_level]++;
    }

    return 0;
}

auto speck::SPECK3D::m_code_S(size_t idx1, size_t idx2) -> int
{
    const auto&               set = m_LIS[idx1][idx2];
    std::array<SPECKSet3D, 8> subsets;
    speck::partition_S_XYZ(set, subsets);
    int rtn = 0;
    for (const auto& s : subsets) {
        if (s.is_pixel()) {
            m_LIP.push_back(s.start_z * m_dim_x * m_dim_y + s.start_y * m_dim_x + s.start_x);
            m_LIP_garbage.push_back(false);
            if (m_encode_mode) {
                if ((rtn = m_process_P_encode(m_LIP.size() - 1)))
                    return rtn;
            } else {
                if ((rtn = m_process_P_decode(m_LIP.size() - 1)))
                    return rtn;
            }
        } else if (!s.is_empty()) {
            const auto newidx1 = s.part_level;
            m_LIS[newidx1].emplace_back(s);
            const auto newidx2 = m_LIS[newidx1].size() - 1;
            if (m_encode_mode) {
                if ((rtn = m_process_S_encode(newidx1, newidx2)))
                    return rtn;
            } else {
                if ((rtn = m_process_S_decode(newidx1, newidx2)))
                    return rtn;
            }
        }
    }

    return 0;
}

auto speck::SPECK3D::m_ready_to_encode() const -> bool
{
    if (m_coeff_buf == nullptr)
        return false;
    if (m_dim_x == 0 || m_dim_y == 0 || m_dim_z == 0)
        return false;
    if (m_budget == 0)
        return false;

    return true;
}

auto speck::SPECK3D::m_ready_to_decode() const -> bool
{
    if (m_bit_buffer.empty())
        return false;
    if (m_dim_x == 0 || m_dim_y == 0 || m_dim_z == 0)
        return false;

    return true;
}

auto speck::SPECK3D::write_to_disk(const std::string& filename) const -> int
{
    // Header definition:
    // information: dim_x,     dim_y,     dim_z,     image_mean,  max_coeff_bits,  bitstream
    // format:      uint32_t,  uint32_t,  uint32_t,  double       int32_t,         packed_bytes
    const size_t header_size = 24;

    // Create and fill header buffer
    size_t   pos = 0;
    uint32_t dims[3] { uint32_t(m_dim_x), uint32_t(m_dim_y), uint32_t(m_dim_z) };
#ifdef NO_CPP14
    buffer_type_c header(new char[header_size]);
#else
    buffer_type_c header = std::make_unique<char[]>(header_size);
#endif
    std::memcpy(header.get(), dims, sizeof(dims));
    pos += sizeof(dims);
    std::memcpy(header.get() + pos, &m_image_mean, sizeof(m_image_mean));
    pos += sizeof(m_image_mean);
    std::memcpy(header.get() + pos, &m_max_coeff_bits, sizeof(m_max_coeff_bits));
    pos += sizeof(m_max_coeff_bits);
    if(pos != header_size)
        return 1;

    // Call the actual write function
    int rtn = m_write(header, header_size, filename.c_str());
    return rtn;
}

auto speck::SPECK3D::read_from_disk(const std::string& filename) -> int
{
    // Header definition:
    // information: dim_x,     dim_y,     dim_z,     image_mean,  max_coeff_bits,  bitstream
    // format:      uint32_t,  uint32_t,  uint32_t,  double       int32_t,         packed_bytes
    const size_t header_size = 24;

    // Create the header buffer, and read from file
    // Note that m_bit_buffer is filled by m_read().
#ifdef NO_CPP14
    buffer_type_c header(new char[header_size]);
#else
    buffer_type_c header = std::make_unique<char[]>(header_size);
#endif
    int rtn = m_read(header, header_size, filename.c_str());
    if (rtn)
        return rtn;

    // Parse the header
    uint32_t dims[3];
    size_t   pos = 0;
    std::memcpy(dims, header.get(), sizeof(dims));
    pos += sizeof(dims);
    std::memcpy(&m_image_mean, header.get() + pos, sizeof(m_image_mean));
    pos += sizeof(m_image_mean);
    std::memcpy(&m_max_coeff_bits, header.get() + pos, sizeof(m_max_coeff_bits));
    pos += sizeof(m_max_coeff_bits);
    if(pos != header_size)
        return 1;

    this->set_dims(size_t(dims[0]), size_t(dims[1]), size_t(dims[2]));

    return 0;
}
