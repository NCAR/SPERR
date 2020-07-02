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
    m_LIS[parts].insert(m_LIS[parts].begin(), big);
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
    for( const auto& o : m_LOS ) {
        if( std::abs( o.err ) < m_tolerance )
            return false;
    }

    return true;
}

auto speck::SPECK3D_Err::m_ready_to_decode() const -> bool
{
    if (m_dim_x == 0 || m_dim_y == 0 || m_dim_z == 0)
        return false;
    if (m_bit_buffer.empty() )
        return false;

    return true;
}

auto speck::SPECK3D_Err::encode() -> int
{
    if ( !m_ready_to_encode() ) 
        return 1;
    m_encode_mode = true;

    // initialize other data structures
    m_initialize_LIS();
    m_outlier_cnt = m_LOS.size(); // Initially everyone in LOS is an outlier
    m_q.clear();
    m_q.reserve(m_LOS.size());
    for (const auto& o : m_LOS)
        m_q.push_back(std::abs(o.err));
    m_err_hat.assign(m_LOS.size(), 0.0f);
    m_pixel_types.assign(m_LOS.size(), PixelType::Insig);
    m_LSP.clear();
    m_LSP.reserve( m_LOS.size() );

    // Find the maximum q, and decide m_max_coeff_bits
    auto max_q       = *(std::max_element(m_q.cbegin(), m_q.cend()));
    auto max_bits_f  = std::floor(std::log2(max_q));
    m_max_coeff_bits = int32_t(max_bits_f);
    m_threshold      = std::pow(2.0f, float(m_max_coeff_bits));

    // Start the iterations!
    for (size_t bitplane = 0; bitplane < 64; bitplane++) {

        if( m_sorting_pass() )
            break;
        if( m_refinement_Sig() )
            break;

        m_threshold *= 0.5f;
    }

#ifdef PRINT
        for( size_t i = 0; i < m_LOS.size(); i++ ) {
            if( m_pixel_types[i] != PixelType::Insig ) {
                auto& out = m_LOS[i];
                printf("  outlier: (%d, %d, %d, %f)\n", out.x, out.y, out.z, m_err_hat[i] );
            }
        }
#endif

    return 0;
}

auto speck::SPECK3D_Err::decode() -> int
{
    if( !m_ready_to_decode() )
        return 1;
    m_encode_mode = false;

    // Clear/initialize data structures
    m_LOS.clear();
    m_LSP.clear();
    m_q.clear();
    m_err_hat.clear();
    m_pixel_types.clear();
    m_initialize_LIS();
    m_bit_idx = 0;

    // Since we already have m_max_coeff_bits from the bit stream, 
    // we can go straight into quantization!
    m_threshold = std::pow(2.0f, float(m_max_coeff_bits));
    for( size_t bitplane = 0; bitplane < 64; bitplane++ ) {

        if( m_sorting_pass() )
            break;
        if( m_refinement_decoding() )
            break;

        m_threshold *= 0.5f;
    }

    // Put restored values in m_LOS with proper signs
    for( size_t idx = 0; idx < m_LOS.size(); idx++ ) {
        m_LOS[idx].err *= m_err_hat[idx];
    }

#ifdef PRINT
        for( size_t i = 0; i < m_LOS.size(); i++ ) {
            if( m_pixel_types[i] != PixelType::Insig ) {
                auto& out = m_LOS[i];
                printf("  outlier: (%d, %d, %d, %f)\n", out.x, out.y, out.z, out.err );
            }
        }
#endif

    return 0;
}

auto speck::SPECK3D_Err::m_decide_significance(const SPECKSet3D& set) const -> int64_t
{
    // Strategy:
    // Iterate all outliers: if
    // 1) its type is PixelType::Insig,
    // 2) its q value is above the current threshold, and
    // 3) its location falls inside this set,
    // then this set is significant. Otherwise its insignificant.

    for (size_t i = 0; i < m_LOS.size(); i++) {

        // Testing condition 1) and 2)
        if (m_pixel_types[i] == PixelType::Insig && m_q[i] >= m_threshold) {

            // Then get ready to test condition 3)
            const auto& olr = m_LOS[i];

            // clang-format off
            if( set.start_x <= olr.x && olr.x < set.start_x + set.length_x &&
                set.start_y <= olr.y && olr.y < set.start_y + set.length_y &&
                set.start_z <= olr.z && olr.z < set.start_z + set.length_z ) {
                return i;
            } // clang-format on
        }
    }

    return -1;
}

auto speck::SPECK3D_Err::m_process_S_encoding(size_t idx1, size_t idx2) -> bool
{
    auto& set     = m_LIS[idx1][idx2];
    auto  sig_idx = m_decide_significance(set);
    bool  is_sig  = sig_idx >= 0;
    m_bit_buffer.push_back(is_sig);

#ifdef PRINT
    std::cout << "threshold = " << m_threshold << ",  set_" << is_sig << std::endl;
#endif

    if (is_sig) {
        if (set.is_pixel()) {
            // Record the sign of this newly identify outlier, and put it in LSP
            m_bit_buffer.push_back( m_LOS[sig_idx].err > 0.0f );
            m_pixel_types[ sig_idx ] = PixelType::NewlySig;
            m_LSP.push_back( sig_idx );
#ifdef PRINT
    std::cout << "threshold = " << m_threshold << ",  sign_" << m_bit_buffer.back() << std::endl;
#endif
            // Refine this pixel!
            if( m_refinement_NewlySig( sig_idx ) )
                return true;
        }
        else {
            if( m_code_S_encoding( idx1, idx2 ) )
                return true;
        }

        set.type = SetType::Garbage;
        m_LIS_garbage_cnt[idx1]++;
    }

    return false;
}

auto speck::SPECK3D_Err::m_code_S_encoding( size_t idx1, size_t idx2 ) -> bool
{
    const auto& set = m_LIS[idx1][idx2];
    std::array<SPECKSet3D, 8> subsets;
    speck::partition_S_XYZ( set, subsets );
    for (const auto& ss : subsets ) {
        if( !ss.is_empty() ) {
            const auto newi1 = ss.part_level;
            m_LIS[newi1].emplace_back( ss );
            const auto newi2 = m_LIS[newi1].size() - 1;
            if( m_process_S_encoding( newi1, newi2 ) )
                return true;
        }
    }

    return false;
}

auto speck::SPECK3D_Err::m_code_S_decoding( size_t idx1, size_t idx2 ) -> bool
{
    const auto& set = m_LIS[idx1][idx2];
    std::array<SPECKSet3D, 8> subsets;
    speck::partition_S_XYZ( set, subsets );
    for (const auto& ss : subsets ) {
        if( !ss.is_empty() ) {
            const auto newi1 = ss.part_level;
            m_LIS[newi1].emplace_back( ss );
            const auto newi2 = m_LIS[newi1].size() - 1;
            if( m_process_S_decoding( newi1, newi2 ) )
                return true;
        }
    }

    return false;
}

auto speck::SPECK3D_Err::m_sorting_pass() -> bool
{
    for( size_t tmp = 1; tmp <= m_LIS.size(); tmp++ ) {
        size_t idx1 = m_LIS.size() - tmp;
        for( size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++ ) {

            const auto& set = m_LIS[idx1][idx2];
            if( set.type != SetType::Garbage ) {
                if( m_encode_mode ) {
                    if( m_process_S_encoding(idx1, idx2) ) 
                        return true;
                } else {
                    if( m_process_S_decoding( idx1, idx2 ) )
                        return true;
                }
            }
        }
    }

    return false;
}

auto speck::SPECK3D_Err::m_refinement_Sig() -> bool
{
    for( auto idx : m_LSP ) {

        if (m_pixel_types[idx] == PixelType::Sig) {

            // First test if this pixel is still an outlier
            auto diff        = m_err_hat[idx] - std::abs(m_LOS[idx].err);
            auto was_outlier = std::abs(diff) > m_tolerance;
            auto need_refine = m_q[idx] >= m_threshold;
            m_bit_buffer.push_back( need_refine );
#ifdef PRINT
    std::cout << "threshold = " << m_threshold << ",  ref_" << m_bit_buffer.back() << std::endl;
#endif
            if (need_refine)
                m_q[idx] -= m_threshold;

            // If this pixel was an outlier, we might have alreayd eliminated it.
            // Let's test to find out! 
            if( was_outlier ) {
                m_err_hat[idx] += need_refine ? m_threshold * 0.5f : m_threshold * -0.5f;
                diff = m_err_hat[idx] - std::abs(m_LOS[idx].err);
                auto is_outlier = std::abs(diff) > m_tolerance;
                if ( !is_outlier ) {
                    m_outlier_cnt--;
                    if (m_outlier_cnt == 0 )
                        return true;
                }
            }
        }
        else if( m_pixel_types[idx] == PixelType::NewlySig ) {
            // Simply remove the NewlySig mark
            m_pixel_types[idx] = PixelType::Sig;
        }
    }

    return false;
}

auto speck::SPECK3D_Err::m_refinement_NewlySig( size_t idx ) -> bool
{
    m_q[idx] -= m_threshold;

    m_err_hat[idx] = m_threshold * 1.5f;
    auto diff = m_err_hat[idx] - std::abs(m_LOS[idx].err);
    auto is_outlier = std::abs(diff) > m_tolerance;
    // Because a NewlySig pixel must be an outlier previously, so we only need to test 
    // if it is currently an outlier.
    if (!is_outlier)
        m_outlier_cnt--;
    if (m_outlier_cnt == 0 )
        return true;
    else
        return false;
}

auto speck::SPECK3D_Err::m_process_S_decoding( size_t idx1, size_t idx2 ) -> bool
{
    auto& set    = m_LIS[idx1][idx2];
    bool  is_sig = m_bit_buffer[m_bit_idx++];

#ifdef PRINT
    std::cout << "threshold = " << m_threshold << ",  set_" << is_sig << std::endl;
#endif
    
    // The bit buffer should NOT be depleted at this point
    assert( m_bit_idx < m_bit_buffer.size() );

    if( is_sig ) {

        if( set.is_pixel() ) {
            // We recovered the location of another outlier! 
            // Is this pixel positive or negative? Keep that info in m_LOS.
            auto sign = m_bit_buffer[m_bit_idx++] ? 1.0f : -1.0f;
            m_LOS.emplace_back( set.start_x, set.start_y, set.start_z, sign );
            m_err_hat.push_back( 1.5f * m_threshold );
            m_pixel_types.push_back( PixelType::NewlySig );
            // Also put this pixel in LSP.
            m_LSP.push_back( m_LOS.size() - 1 );

#ifdef PRINT
    std::cout << "threshold = " << m_threshold << ",  sign_" << m_bit_buffer[m_bit_idx-1] << std::endl;
#endif
            
            // The bit buffer CAN be depleted at this point, so let's do a test
            if( m_bit_idx == m_bit_buffer.size() )
                return true;
        } else {
            if( m_code_S_decoding( idx1, idx2 ) )
                return true;
        }

        set.type = SetType::Garbage;
        m_LIS_garbage_cnt[idx1]++;
    }

    return false;
}

auto speck::SPECK3D_Err::m_refinement_decoding() -> bool
{
    // sanity check
    assert( m_LOS.size() == m_err_hat.size() );
    assert( m_LOS.size() == m_pixel_types.size() );

    //for( size_t idx = 0; idx < m_LOS.size(); idx++ ) {
    for( auto idx : m_LSP ) {

        if( m_pixel_types[idx] == PixelType::Sig ) {

            // Existing significant pixels are further refined
            if( m_bit_buffer[m_bit_idx++] )
                m_err_hat[idx] += m_threshold * 0.5f;
            else
                m_err_hat[idx] -= m_threshold * 0.5f;

#ifdef PRINT
    std::cout << "threshold = " << m_threshold << ",  ref_" << m_bit_buffer[m_bit_idx-1] << std::endl;
#endif

            if( m_bit_idx == m_bit_buffer.size() )
                return true;

        } else {
            assert( m_pixel_types[idx] == PixelType::NewlySig ); // Sanity check

            // For NewlySig pixels, just mark it as significant
            m_pixel_types[idx] = PixelType::Sig;
        }
    }

    return false;
}

auto speck::SPECK3D_Err::get_num_of_outliers() const -> size_t
{
    return m_LOS.size();
}

auto speck::SPECK3D_Err::get_ith_outlier( size_t idx ) const -> Outlier
{
    return m_LOS[idx];
}

auto speck::SPECK3D_Err::release_outliers() -> std::vector<Outlier>
{
    return std::move( m_LOS );
}

