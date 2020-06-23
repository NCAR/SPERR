#include "SPECK3D_Err.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>

#define PRINT

void speck::SPECK3D_Err::reserve(size_t num)
{
    m_LOS.reserve(num);
    m_q.reserve(num);
    m_err_hat.reserve(num);
    m_pixel_types.reserve(num);
}

void speck::SPECK3D_Err::add_outlier(uint32_t x, uint32_t y, uint32_t z, float e)
{
    m_LOS.emplace_back(x, y, z, e);
}

void speck::SPECK3D_Err::add_outlier_list(std::vector<Outlier> list)
{
    m_LOS = std::move(list);
}

void speck::SPECK3D_Err::set_dims(size_t x, size_t y, size_t z)
{
    m_dim_x = x;
    m_dim_y = y;
    m_dim_z = z;
}

void speck::SPECK3D_Err::set_tolerance(float t)
{
    m_tolerance = t;
}

void speck::SPECK3D_Err::m_initialize_LIS()
{
    std::array<size_t, 3> num_of_parts; // how many times each dimension could be partitioned?
    num_of_parts[0]     = speck::num_of_partitions(m_dim_x);
    num_of_parts[1]     = speck::num_of_partitions(m_dim_y);
    num_of_parts[2]     = speck::num_of_partitions(m_dim_z);
    size_t num_of_sizes = 1;
    for (size_t i = 0; i < 3; i++)
        num_of_sizes += num_of_parts[i];

    // initialize LIS
    m_LIS.clear();
    m_LIS.resize(num_of_sizes);
    m_LIS_garbage_cnt.assign(num_of_sizes, 0);

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
}

void speck::SPECK3D_Err::m_clean_LIS()
{
    std::vector<SPECKSet3D> tmpv;

    for (size_t tmpi = 1; tmpi <= m_LIS_garbage_cnt.size(); tmpi++) {

        // Because lists towards the end tend to have bigger sizes, we look at
        // them first. This practices should reduce the number of memory allocations.
        const auto idx = m_LIS_garbage_cnt.size() - tmpi;

        // Only consolidate memory if the garbage count is more than half
        if (m_LIS_garbage_cnt[idx] > m_LIS[idx].size() / 2) {
            auto& list = m_LIS[idx];
            tmpv.clear();
            tmpv.reserve(list.size()); // will leave half capacity unfilled, so the list
                                       // won't need a memory re-allocation for a while.
            std::copy_if(list.cbegin(), list.cend(), std::back_inserter(tmpv),
                         [](const SPECKSet3D& s) { return s.type != SetType::Garbage; });
            std::swap(list, tmpv);
            m_LIS_garbage_cnt[idx] = 0;
        }
    }
}

auto speck::SPECK3D_Err::m_ready_to_encode() const -> bool
{
    if (m_dim_x == 0 || m_dim_y == 0 || m_dim_z == 0)
        return false;
    if (m_tolerance <= 0.0f)
        return false;
    if (m_LOS.empty())
        return false;

    return true;
}

auto speck::SPECK3D_Err::encode() -> int
{
    if ( !m_ready_to_encode() ) 
        return 1;
    m_encode_mode = true;

    // Process all outliers by subtracting m_tolerance.
    // The result is that all outliers are closer to the origin, while 
    // still maintaining their signs.
    for (auto& o : m_LOS) {
        if( o.err > 0.0f )
            o.err -= m_tolerance;
        else
            o.err += m_tolerance;
    }

    // initialize other data structures
    m_initialize_LIS();
    m_outlier_cnt = m_LOS.size(); // Initially everyone in LOS is an outlier
    m_q.reserve(m_outlier_cnt);
    for (const auto& o : m_LOS)
        m_q.push_back(std::abs(o.err));
    m_err_hat.assign(m_outlier_cnt, 0.0f);
    m_pixel_types.assign(m_outlier_cnt, PixelType::Insig);

    // Find the maximum q, and decide m_max_coeff_bits
    auto max_q       = *(std::max_element(m_q.cbegin(), m_q.cend()));
    auto max_bits_f  = std::floor(std::log2(max_q));
    m_max_coeff_bits = int32_t(max_bits_f);
    m_threshold      = std::pow(2.0f, float(m_max_coeff_bits));

    // Start the iterations!
    for (size_t bitplane = 0; bitplane < 128; bitplane++) {

        m_sorting_pass();
        if( m_refinement_pass() )
            break;

        m_threshold *= 0.5f;
    }

    return 0;
}

auto speck::SPECK3D_Err::m_decide_significance(const SPECKSet3D& set) const -> bool
{
    // Strategy:
    // Iterate all outliers: if
    // 1) its type is PixelType::Insig,
    // 2) its q value is above the current threshold, and
    // 3) its location falls inside this set,
    // then this set is significant. Otherwise its insignificant.

    for (size_t i = 0; i < m_pixel_types.size(); i++) {
        // Testing condition 1) and 2)
        if (m_pixel_types[i] == PixelType::Insig && m_q[i] >= m_threshold) {
            const auto& olr = m_LOS[i];

            // Testing condition 3)
            // clang-format off
            if( set.start_x <= olr.x && olr.x < set.start_x + set.length_x &&
                set.start_y <= olr.y && olr.y < set.start_y + set.length_y &&
                set.start_z <= olr.z && olr.z < set.start_z + set.length_z ) {
                return true;
            } // clang-format on
        }
    }

    return false;
}

void speck::SPECK3D_Err::m_process_S(size_t idx1, size_t idx2)
{
    auto& set    = m_LIS[idx1][idx2];
    bool  is_sig = m_decide_significance(set);
    m_bit_buffer.push_back(is_sig);
#ifdef PRINT
    std::cout << "threshold = " << m_threshold << ",  p" << is_sig << std::endl;
#endif

    if (is_sig) {
        if (set.is_pixel()) {

            // Need to first find the index of this pixel in m_LOS
            int64_t idx = -1;
            for (int64_t i = 0; i < m_LOS.size(); i++) {
                // clang-format off
                if( m_pixel_types[i] == PixelType::Insig && 
                    m_LOS[i].x       == set.start_x      && 
                    m_LOS[i].y       == set.start_y      && 
                    m_LOS[i].z       == set.start_z       ) {
                    idx = i;
                    break;
                } // clang-format off
                assert( idx != -1 );
            }
            m_bit_buffer.push_back( m_LOS[idx].err >= 0.0f );
            m_pixel_types[ idx ] = PixelType::NewlySig;
#ifdef PRINT
    std::cout << "threshold = " << m_threshold << ",  s" << m_bit_buffer.back() << std::endl;
#endif
        }
        else {
            m_code_S( idx1, idx2 );
        }

        set.type = SetType::Garbage;
        m_LIS_garbage_cnt[idx1]++;
    }
}

void speck::SPECK3D_Err::m_code_S( size_t idx1, size_t idx2 )
{
    const auto& set = m_LIS[idx1][idx2];
    std::array<SPECKSet3D, 8> subsets;
    speck::partition_S_XYZ( set, subsets );
    for (const auto& ss : subsets ) {
        if( !ss.is_empty() ) {
            const auto newi1 = ss.part_level;
            m_LIS[newi1].emplace_back( ss );
            const auto newi2 = m_LIS[newi1].size() - 1;
            m_process_S( newi1, newi2 );
        }
    }
}

void speck::SPECK3D_Err::m_sorting_pass()
{
    for( size_t tmp = 1; tmp <= m_LIS.size(); tmp++ ) {
        size_t idx1 = m_LIS.size() - tmp;
        for( size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++ ) {
            const auto& set = m_LIS[idx1][idx2];
            if( set.type != SetType::Garbage ) {
                m_process_S( idx1, idx2 );
            }
        }
    }
}

auto speck::SPECK3D_Err::m_refinement_pass() -> bool
{
    for( size_t idx = 0; idx < m_pixel_types.size(); idx++ ) {

        // LSP is implicitly indicated by m_pixel_types
        if (m_pixel_types[idx] == PixelType::Sig) {
            auto was_outlier = (std::abs( m_err_hat[idx] - m_LOS[idx].err ) > m_tolerance);
            auto need_refine = (m_q[idx] >= m_threshold);
            m_bit_buffer.push_back( need_refine );
#ifdef PRINT
    std::cout << "threshold = " << m_threshold << ",  r" << m_bit_buffer.back() << std::endl;
#endif
            if (need_refine) {
                m_q[idx] -= m_threshold;
                m_err_hat[idx] += m_threshold * 0.5f;
            }
            else {
                m_err_hat[idx] += m_threshold * -0.5f;
            }

            auto is_outlier = (std::abs( m_err_hat[idx] - m_LOS[idx].err ) > m_tolerance);
            // If error at this idx is reduced by this iteration of refinement,
            // we can decrease m_outlier_cnt.
            if ( was_outlier && !is_outlier )
                m_outlier_cnt--;
        }
        else if( m_pixel_types[idx] == PixelType::NewlySig ) {
            m_q[idx] -= m_threshold;
            m_pixel_types[idx] = PixelType::Sig;

            m_err_hat[idx] = m_threshold * 1.5f;
            auto is_outlier = (std::abs( m_err_hat[idx] - m_LOS[idx].err ) > m_tolerance);
            // Because a NewlySig pixel must be an outlier previously, so we only need to test 
            // if it is currently an outlier.
            if (!is_outlier)
                m_outlier_cnt--;
        }

        if (m_outlier_cnt == 0 )
            return true;
    }

    return false;
}


