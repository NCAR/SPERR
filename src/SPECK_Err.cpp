#include "SPECK_Err.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>

// #define PRINT

void speck::SPECK_Err::reserve(size_t num)
{
    m_LOS.reserve(num);
    m_q.reserve(num);
    m_err_hat.reserve(num);
    m_pixel_types.reserve(num);
}

void speck::SPECK_Err::add_outlier(size_t pos, float e)
{
    m_LOS.emplace_back(pos, e);
}

void speck::SPECK_Err::add_outlier_list(std::vector<Outlier> list)
{
    m_LOS = std::move(list);
}

void speck::SPECK_Err::set_length(size_t len )
{
    m_total_len = len;
}

void speck::SPECK_Err::set_tolerance(float t)
{
    m_tolerance = t;
}

auto speck::SPECK_Err::m_part_set( const SPECKSet1D& set ) const -> TwoSets
{
    TwoSets set01;
    // Prepare the 1st set
    set01[0].start      = set.start;
    set01[0].length     = set.length / 2;
    set01[0].part_level = set.part_level + 1;
    // Prepare the 2nd set
    set01[1].start      = set01[0].start + set01[0].length;
    set01[1].length     = set.length - set01[0].length;
    set01[1].part_level = set.part_level + 1;

    return set01;
}

void speck::SPECK_Err::m_initialize_LIS()
{
    // To initialize this 1D array, just calculate how many partitions can be performed,
    // and partition this long array to 2 parts.

    // Initialize LIS with possible length values
    auto num_of_parts = speck::num_of_partitions( m_total_len );
    auto num_of_sizes = num_of_parts + 1;
    m_LIS.clear();
    m_LIS.resize(num_of_sizes);
    m_LIS_garbage_cnt.assign(num_of_sizes, 0);

    // Put in two sets, each representing a half of the long array.
    SPECKSet1D set{ 0, m_total_len, 0 };    // the whole 1D array
    auto set01 = m_part_set( set );
    m_LIS[ 1 ].push_back( set01[0] );
    m_LIS[ 1 ].push_back( set01[1] );
}

void speck::SPECK_Err::m_clean_LIS()
{
    std::vector<SPECKSet1D> tmpv;

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
                         [](const SPECKSet1D& s) { return s.type != SetType::Garbage; });
            std::swap(list, tmpv);
            m_LIS_garbage_cnt[idx] = 0;
        }
    }
}

auto speck::SPECK_Err::m_ready_to_encode() const -> bool
{
    if (m_total_len == 0 )
        return false;
    if (m_tolerance <= 0.0f)
        return false;
    if (m_LOS.empty())
        return false;
    for( const auto& o : m_LOS ) {
        if( std::abs( o.error ) <= m_tolerance )
            return false;
    }

    return true;
}

auto speck::SPECK_Err::m_ready_to_decode() const -> bool
{
    if (m_total_len == 0 )
        return false;
    if (m_bit_buffer.empty() )
        return false;

    return true;
}

auto speck::SPECK_Err::encode() -> int
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
        m_q.push_back(std::abs(o.error));
    m_err_hat.assign(m_LOS.size(), 0.0f);
    m_pixel_types.assign(m_LOS.size(), Significance::Insig);
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
        if( m_pixel_types[i] == Significance::Sig || 
            m_pixel_types[i] == Significance::NewlySig ) {
            auto& out = m_LOS[i];
            printf("  outlier: (%ld, %f)\n", out.location, m_err_hat[i] );
        }
    }
#endif

    return 0;
}

auto speck::SPECK_Err::decode() -> int
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

    // Since we already have m_max_coeff_bits from the bit stream when decoding header,
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
        m_LOS[idx].error *= m_err_hat[idx];
    }

#ifdef PRINT
    for( size_t i = 0; i < m_LOS.size(); i++ ) {
        if( m_pixel_types[i] == Significance::Sig || 
            m_pixel_types[i] == Significance::NewlySig ) {
            auto& out = m_LOS[i];
            printf("  outlier: (%ld, %f)\n", out.location, out.error );
        }
    }
#endif

    return 0;
}

auto speck::SPECK_Err::m_decide_significance(const SPECKSet1D& set) const -> int64_t
{
    // Strategy:
    // Iterate all outliers: if
    // 1) its type is Significance::Insig,
    // 2) its q value is above the current threshold, and
    // 3) its location falls inside this set,
    // then this set is significant. Otherwise its insignificant.

    for (size_t i = 0; i < m_LOS.size(); i++) {

        if (m_pixel_types[i] == Significance::Insig && m_q[i] >= m_threshold &&
            set.start <= m_LOS[i].location && m_LOS[i].location < set.start + set.length )
                return i;
    }

    return -1;
}

auto speck::SPECK_Err::m_process_S_encoding(size_t idx1, size_t idx2) -> bool
{
    auto& set     = m_LIS[idx1][idx2];
    auto  sig_idx = m_decide_significance(set);
    bool  is_sig  = sig_idx >= 0;
    m_bit_buffer.push_back(is_sig);

#ifdef PRINT
    std::cout << "threshold = " << m_threshold << ",  set_" << is_sig << std::endl;
#endif

    if (is_sig) {
        if (set.length == 1) { // Is a pixel
            // Record the sign of this newly identify outlier, and put it in LSP
            m_bit_buffer.push_back( m_LOS[sig_idx].error > 0.0f );
            m_pixel_types[ sig_idx ] = Significance::NewlySig;
            m_LSP.push_back( sig_idx );
#ifdef PRINT
    std::cout << "threshold = " << m_threshold << ",  sign_" << m_bit_buffer.back() << std::endl;
#endif
            // Refine this pixel!
            if( m_refinement_NewlySig( sig_idx ) )
                return true;
        }
        else {
            if( m_code_S( idx1, idx2 ) )
                return true;
        }

        set.type = SetType::Garbage;
        m_LIS_garbage_cnt[idx1]++;
    }

    return false;
}

auto speck::SPECK_Err::m_code_S( size_t idx1, size_t idx2 ) -> bool
{
    const auto& set = m_LIS[idx1][idx2];

    auto set01 = m_part_set( set );
    for( const auto& ss : set01 ) {
        if( ss.length > 0 ) {
            auto newi1 = ss.part_level;
            m_LIS[newi1].emplace_back( ss );
            auto newi2 = m_LIS[newi1].size() - 1;
            if( m_encode_mode && m_process_S_encoding( newi1, newi2 ) )
                return true;
            else if( !m_encode_mode && m_process_S_decoding( newi1, newi2 ) )
                return true;
        }
    }

    return false;
}

auto speck::SPECK_Err::m_sorting_pass() -> bool
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

auto speck::SPECK_Err::m_refinement_Sig() -> bool
{
    for( auto idx : m_LSP ) {

        if (m_pixel_types[idx] == Significance::Sig) {

            // First test if this pixel is still an outlier
            auto diff        = m_err_hat[idx] - std::abs(m_LOS[idx].error);
            auto was_outlier = std::abs(diff) > m_tolerance;
            auto need_refine = m_q[idx] >= m_threshold;
            m_bit_buffer.push_back( need_refine );
#ifdef PRINT
    std::cout << "threshold = " << m_threshold << ",  ref_" << m_bit_buffer.back() << std::endl;
#endif
            if (need_refine)
                m_q[idx] -= m_threshold;

            // If this pixel was an outlier, we refine it, and test again!
            if( was_outlier ) {
                m_err_hat[idx] += need_refine ? m_threshold * 0.5f : m_threshold * -0.5f;
                diff = m_err_hat[idx] - std::abs(m_LOS[idx].error);
                auto is_outlier = std::abs(diff) > m_tolerance;
                if ( !is_outlier ) {
                    m_outlier_cnt--;
                    if (m_outlier_cnt == 0 )
                        return true;
                }
            }
        }
        else if( m_pixel_types[idx] == Significance::NewlySig ) {
            // Simply remove the NewlySig mark
            m_pixel_types[idx] = Significance::Sig;
        }
    }

    return false;
}

auto speck::SPECK_Err::m_refinement_NewlySig( size_t idx ) -> bool
{
    m_q[idx] -= m_threshold;

    m_err_hat[idx] = m_threshold * 1.5f;
    auto diff = m_err_hat[idx] - std::abs(m_LOS[idx].error);
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

auto speck::SPECK_Err::m_process_S_decoding( size_t idx1, size_t idx2 ) -> bool
{
    auto& set    = m_LIS[idx1][idx2];
    bool  is_sig = m_bit_buffer[m_bit_idx++];

#ifdef PRINT
    std::cout << "threshold = " << m_threshold << ",  set_" << is_sig << std::endl;
#endif
    
    // The bit buffer should NOT be depleted at this point
    assert( m_bit_idx < m_bit_buffer.size() );

    if( is_sig ) {

        if( set.length == 1 ) // This is a pixel
        {
            // We recovered the location of another outlier! 
            // Is this pixel positive or negative? Keep that info in m_LOS.
            auto sign = m_bit_buffer[m_bit_idx++] ? 1.0f : -1.0f;
            m_LOS.emplace_back( set.start, sign );
            m_err_hat.push_back( 1.5f * m_threshold );
            m_pixel_types.push_back( Significance::NewlySig );
            // Also put this pixel in LSP.
            // TODO: m_LSP seems unnecessary when decoding. 
            m_LSP.push_back( m_LOS.size() - 1 );

#ifdef PRINT
    std::cout << "threshold = " << m_threshold << ",  sign_" << m_bit_buffer[m_bit_idx-1] << std::endl;
#endif
            
            // The bit buffer CAN be depleted at this point, so let's do a test
            if( m_bit_idx == m_bit_buffer.size() )
                return true;
        } else {
            if( m_code_S( idx1, idx2 ) )
                return true;
        }

        set.type = SetType::Garbage;
        m_LIS_garbage_cnt[idx1]++;
    }

    return false;
}

auto speck::SPECK_Err::m_refinement_decoding() -> bool
{
    // sanity check
    assert( m_LOS.size() == m_err_hat.size() );
    assert( m_LOS.size() == m_pixel_types.size() );

    for( auto idx : m_LSP ) {

        if( m_pixel_types[idx] == Significance::Sig ) {

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
            assert( m_pixel_types[idx] == Significance::NewlySig ); // Sanity check

            // For NewlySig pixels, just mark it as significant
            m_pixel_types[idx] = Significance::Sig;
        }
    }

    return false;
}

auto speck::SPECK_Err::get_num_of_outliers() const -> size_t
{
    return m_LOS.size();
}

auto speck::SPECK_Err::num_of_bits() const -> size_t
{
    return m_bit_buffer.size();
}

auto speck::SPECK_Err::get_ith_outlier( size_t idx ) const -> Outlier
{
    return m_LOS[idx];
}

auto speck::SPECK_Err::release_outliers() -> std::vector<Outlier>
{
    return std::move( m_LOS );
}

