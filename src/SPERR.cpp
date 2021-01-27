#include "SPERR.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>

//
// Class SPECKSet1D
//
speck::SPECKSet1D::SPECKSet1D( size_t s, size_t l, uint32_t p )
                 : start(s), length(l), part_level(p)
{ }

//
// Struct Outlier
//
speck::Outlier::Outlier( size_t loc, double e )
              : location(loc), error(e)
{ }


//
// Class SPERR
//
void speck::SPERR::add_outlier(size_t pos, double e)
{
    m_LOS.emplace_back(pos, e);
}

void speck::SPERR::use_outlier_list(std::vector<Outlier> list)
{
    m_LOS = std::move(list);
}

void speck::SPERR::set_length(size_t len)
{
    m_total_len = len;
}

void speck::SPERR::set_tolerance(double t)
{
    m_tolerance = t;
}

void speck::SPERR::use_bit_buffer( std::vector<bool> other_buf )
{
    m_bit_buffer = std::move( other_buf );
}

auto speck::SPERR::release_bit_buffer() -> std::vector<bool>
{
    // Do some clean up work first
    m_LOS.clear();
    m_total_len = 0;
    m_tolerance = 0.0;
    return std::move(m_bit_buffer);
}

auto speck::SPERR::m_part_set(const SPECKSet1D& set) const -> TwoSets 
{
    SPECKSet1D set1, set2;
    // Prepare the 1st set
    set1.start      = set.start;
    set1.length     = set.length / 2;
    set1.part_level = set.part_level + 1;
    // Prepare the 2nd set
    set2.start      = set1.start + set1.length;
    set2.length     = set.length - set1.length;
    set2.part_level = set.part_level + 1;

    return {set1, set2};
}

void speck::SPERR::m_initialize_LIS()
{
    // To initialize this 1D array, just calculate how many partitions can be performed,
    // and partition this long array to 2 parts.

    // Initialize LIS with possible length values
    auto num_of_parts = speck::num_of_partitions(m_total_len);
    auto num_of_sizes = num_of_parts + 1;
    m_LIS.clear();
    m_LIS.resize(num_of_sizes);

    // Put in two sets, each representing a half of the long array.
    SPECKSet1D set ( 0, m_total_len, 0 ); // the whole 1D array
    auto sets = m_part_set(set);
    m_LIS[sets[0].part_level].push_back(sets[0]);
    m_LIS[sets[1].part_level].push_back(sets[1]);
}

void speck::SPERR::m_clean_LIS()
{
    for( auto& list : m_LIS ) {
        auto itr = std::remove_if( list.begin(), list.end(), 
                                   [](const auto& s) { return s.type == SetType::Garbage; });
        list.erase( itr, list.end() );
    }
}

auto speck::SPERR::m_ready_to_encode() const -> bool
{
    if (m_total_len == 0)
        return false;
    if (m_tolerance <= 0.0)
        return false;
    if (m_LOS.empty())
        return false;

    return std::all_of( m_LOS.begin(), m_LOS.end(),
                        [len = m_total_len, tol = m_tolerance](auto& out)
                        {return out.location < len && std::abs(out.error) > tol;} );
}

auto speck::SPERR::m_ready_to_decode() const -> bool
{
    if (m_total_len == 0)
        return false;
    if (m_bit_buffer.empty())
        return false;

    return true;
}

auto speck::SPERR::encode() -> RTNType
{
    if (!m_ready_to_encode())
        return RTNType::InvalidParam;
    m_encode_mode = true;

    m_initialize_LIS();
    m_outlier_cnt = m_LOS.size(); // Initially everyone in LOS is an outlier
    m_q.assign( m_LOS.size(), 0.0 );

    // m_q is initialized to have the absolute value of all error.
    std::transform( m_LOS.cbegin(), m_LOS.cend(), m_q.begin(),
                    [](auto& out){return std::abs(out.error);} );

    // m_err_hat is initialized to have zeros.
    m_err_hat.assign(m_LOS.size(), 0.0);

    m_LSP_new.clear();
    m_LSP_old.clear();
    m_LSP_old.reserve(m_LOS.size());

    // Find the maximum q, and decide m_max_coeff_bits
    auto max_q       = *(std::max_element(m_q.cbegin(), m_q.cend()));
    auto max_bits    = std::floor(std::log2(max_q));
    m_max_coeff_bits = int32_t(max_bits);
    m_threshold      = std::pow(2.0, double(m_max_coeff_bits));

    // Start the iterations!
    for (size_t bitplane = 0; bitplane < 64; bitplane++) {

        if (m_sorting_pass())
            break;
        if (m_refinement_pass_encoding())
            break;

        m_threshold *= 0.5;
        m_clean_LIS();
    }

    return RTNType::Good;
}

auto speck::SPERR::decode() -> RTNType
{
    if (!m_ready_to_decode())
        return RTNType::InvalidParam;
    m_encode_mode = false;

    // Clear/initialize data structures
    m_LOS.clear();
    m_recovered_signs.clear();
    m_initialize_LIS();
    m_bit_idx  = 0;
    m_LOS_size = 0;

    // Since we already have m_max_coeff_bits from the bit stream when decoding header,
    // we can go straight into quantization!
    m_threshold = std::pow(2.0, double(m_max_coeff_bits));

    for (size_t bitplane = 0; bitplane < 64; bitplane++) {

        if (m_sorting_pass())
            break;
        if (m_refinement_decoding())
            break;

        m_threshold *= 0.5;
        m_clean_LIS();
    }

    // Put restored values in m_LOS with proper signs
    assert( m_LOS.size() == m_recovered_signs.size() );
    for (size_t idx = 0; idx < m_LOS.size(); idx++) {
        if( m_recovered_signs[idx] == false ) {
            m_LOS[idx].error = -m_LOS[idx].error;
        }
    }

    return RTNType::Good;
}

auto speck::SPERR::m_decide_significance(const SPECKSet1D& set) const 
                       -> std::pair<bool, size_t>
{
    // This function is only used during encoding.
    // Strategy:
    // Iterate all outliers: if
    // 1) its err_hat value is 0.0, meaning it's not processed yet,
    // 2) its q value is above the current threshold, and
    // 3) its location falls inside this set,
    // then this set is significant. Otherwise its insignificant.

    for (size_t i = 0; i < m_LOS.size(); i++) {

        if (m_err_hat[i] == 0.0 && m_q[i] >= m_threshold && 
            set.start <= m_LOS[i].location && m_LOS[i].location < set.start + set.length)
            return {true, i};
    }

    return {false, 0};
}

auto speck::SPERR::m_process_S_encoding(size_t idx1, size_t idx2) -> bool
{
    auto& set     = m_LIS[idx1][idx2];
    auto  sig_rtn = m_decide_significance(set);
    bool  is_sig  = sig_rtn.first;
    auto  sig_idx = sig_rtn.second;
    m_bit_buffer.push_back( is_sig );

    if (is_sig) {
        if (set.length == 1) { // Is a pixel
            // Record the sign of this newly identify outlier, and put it in LSP
            m_bit_buffer.push_back(m_LOS[sig_idx].error >= 0.0);
            m_LSP_new.push_back(sig_idx);

            // Refine this pixel!
            if (m_refinement_new_SP(sig_idx))
                return true;
        }
        else { // Not a pixel
            if (m_code_S(idx1, idx2))
                return true;
        }

        set.type = SetType::Garbage;
    }

    return false;
}

auto speck::SPERR::m_code_S(size_t idx1, size_t idx2) -> bool
{
    const auto& set = m_LIS[idx1][idx2];

    auto sets = m_part_set(set);
    for( const auto& ss : sets ) {
        if (ss.length > 0) {
            auto newi1 = ss.part_level;
            m_LIS[newi1].emplace_back(ss);
            auto newi2 = m_LIS[newi1].size() - 1;
            if (m_encode_mode && m_process_S_encoding(newi1, newi2))
                return true;
            else if (!m_encode_mode && m_process_S_decoding(newi1, newi2))
                return true;
        }
    }

    return false;
}

auto speck::SPERR::m_sorting_pass() -> bool
{
    for (size_t tmp = 1; tmp <= m_LIS.size(); tmp++) {
        size_t idx1 = m_LIS.size() - tmp;
        for (size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++) {
            if (m_encode_mode && m_process_S_encoding(idx1, idx2))
                return true;
            else if (!m_encode_mode && m_process_S_decoding(idx1, idx2))
                return true;
        }
    }

    return false;
}

auto speck::SPERR::m_refinement_pass_encoding() -> bool
{
    for (auto idx : m_LSP_old) {

        // First test if this pixel is still an outlier
        auto diff        = m_err_hat[idx] - std::abs(m_LOS[idx].error);
        auto was_outlier = std::abs(diff) > m_tolerance;
        auto need_refine = m_q[idx] >= m_threshold;
        m_bit_buffer.push_back(need_refine);
        if (need_refine)
            m_q[idx] -= m_threshold;

        // If this pixel was an outlier, we test again!
        if (was_outlier) {
            m_err_hat[idx] += need_refine ? m_threshold * 0.5 : m_threshold * -0.5;
            diff            = m_err_hat[idx] - std::abs(m_LOS[idx].error);
            auto is_outlier = std::abs(diff) > m_tolerance;
            if (!is_outlier) {
                m_outlier_cnt--;
                if (m_outlier_cnt == 0)
                    return true;
            }
        }
    }

    // Now attach content from `m_LSP_new` to the end of `m_LSP_old`, and 
    // marking those pixels as significant.
    m_LSP_old.insert( m_LSP_old.end(), m_LSP_new.begin(), m_LSP_new.end() );
    m_LSP_new.clear();

    return false;
}

auto speck::SPERR::m_refinement_new_SP(size_t idx) -> bool
{
    m_q[idx] -= m_threshold;

    m_err_hat[idx] = m_threshold * 1.5;

    // Because a NewlySig pixel must be an outlier previously, so we only need to test
    // if it is currently an outlier.
    auto diff       = m_err_hat[idx] - std::abs(m_LOS[idx].error);
    auto is_outlier = std::abs(diff) > m_tolerance;
    if (!is_outlier)
        m_outlier_cnt--;

    if (m_outlier_cnt == 0)
        return true;
    else
        return false;
}

auto speck::SPERR::m_process_S_decoding(size_t idx1, size_t idx2) -> bool
{
    auto& set    = m_LIS[idx1][idx2];
    bool  is_sig = m_bit_buffer[m_bit_idx++];

    // Sanity check: the bit buffer should NOT be depleted at this point
    assert(m_bit_idx < m_bit_buffer.size());

    if (is_sig) {
        if (set.length == 1) { // This is a pixel
            // We recovered the location of another outlier!
            m_LOS.emplace_back(set.start, m_threshold * 1.5);
            m_recovered_signs.push_back( m_bit_buffer[m_bit_idx++] );

            // The bit buffer CAN be depleted at this point, so let's do a test
            if (m_bit_idx == m_bit_buffer.size())
                return true;
        } 
        else {
            if (m_code_S(idx1, idx2))
                return true;
        }

        set.type = SetType::Garbage;
    }

    return false;
}

auto speck::SPERR::m_refinement_decoding() -> bool
{
    // Refine significant pixels from previous iterations only, 
    //   because pixels added from this iteration are already refined.
    for (size_t idx = 0; idx < m_LOS_size; idx++) {
        if (m_bit_buffer[m_bit_idx++])
            m_LOS[idx].error += m_threshold * 0.5;
        else
            m_LOS[idx].error -= m_threshold * 0.5;

        if (m_bit_idx == m_bit_buffer.size())
            return true;
    }

    // Record the size of `m_LOS` after this iteration.
    m_LOS_size = m_LOS.size();

    return false;
}

auto speck::SPERR::num_of_outliers() const -> size_t
{
    return m_LOS.size();
}

auto speck::SPERR::num_of_bits() const -> size_t
{
    return m_bit_buffer.size();
}

auto speck::SPERR::ith_outlier(size_t idx) const -> Outlier
{
    return m_LOS[idx];
}

auto speck::SPERR::max_coeff_bits() const -> int32_t
{
    return m_max_coeff_bits;
}

auto speck::SPERR::release_outliers() -> std::vector<Outlier>
{
    return std::move(m_LOS);
}
