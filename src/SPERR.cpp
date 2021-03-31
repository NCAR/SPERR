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

void speck::SPERR::use_outlier_list( const std::vector<Outlier>& list)
{
    m_LOS.resize( list.size() );
    std::copy( list.begin(), list.end(), m_LOS.begin() );
}

void speck::SPERR::set_length(uint64_t len)
{
    m_total_len = len;
}

void speck::SPERR::set_tolerance(double t)
{
    m_tolerance = t;
}

auto speck::SPERR::release_outliers() -> std::vector<Outlier>
{
    return std::move(m_LOS);
}
auto speck::SPERR::view_outliers() -> const std::vector<Outlier>&
{
    return m_LOS;
}


auto speck::SPERR::m_part_set(const SPECKSet1D& set) const -> std::array<SPECKSet1D, 2>
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
    // Note that `m_LIS` is a 2D array. In order to avoid unnecessary memory allocation, 
    // we don't clear `m_LIS` itself, but clear every secondary array it holds.
    // This is OK as long as the extra secondary arrays are cleared.
    auto num_of_parts = speck::num_of_partitions(m_total_len);
    auto num_of_lists = num_of_parts + 1;
    if( m_LIS.size() < num_of_lists )
        m_LIS.resize( num_of_lists );
    for( auto& list : m_LIS )
        list.clear();

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
                    [](auto& s){return s.type == SetType::Garbage;});
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

    // Make sure each outlier to process is a valid outlier.
    if( std::any_of( m_LOS.begin(), m_LOS.end(),
        [len = m_total_len, tol = m_tolerance](auto& out)
        {return out.location >= len || std::abs(out.error) <= tol;}))
        return false;

    // Make sure there are no duplicate locations in the outlier list.
    // Note that the list of outliers is already sorted at the beginning of encoding.
    auto adj = std::adjacent_find( m_LOS.begin(), m_LOS.end(), [](auto& a, auto& b)
                                   {return a.location == b.location;} );
    return (adj == m_LOS.end());
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
    // Let's sort the list of outliers so it'll be easier to locate particular individuals.
    std::sort( m_LOS.begin(), m_LOS.end(), [](auto& a, auto& b){return a.location < b.location;} );
    if (!m_ready_to_encode())
        return RTNType::InvalidParam;
    m_bit_buffer.clear();
    m_encode_mode = true;

    m_initialize_LIS();
    m_outlier_cnt = m_LOS.size(); // Initially everyone in LOS is an outlier

    // `m_q` is initialized to have the absolute value of all error.
    m_q.assign( m_LOS.size(), 0.0 );
    std::transform( m_LOS.cbegin(), m_LOS.cend(), m_q.begin(),
                    [](auto& out){return std::abs(out.error);} );

    // `m_err_hat` is initialized to have zeros.
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

        // Reset the significance map
        m_sig_map.assign(m_total_len, false);
        for( size_t i = 0; i < m_LOS.size(); i++ ) {
            if( m_q[i] >= m_threshold )
                m_sig_map[ m_LOS[i].location ] = true;
        }

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

    // Step 1: use the significance map to decide if this set is significant
    std::pair<bool, size_t> sig{false, 0};
    for( size_t i = set.start; i < set.start + set.length; i++ ) {
        if( m_sig_map[i] ) {
            sig = {true, i};
            break;
        }
    }

    // Step 2: if this set is significant, then find the index of the outlier in 
    //         `m_LSO` that caused it being significant.
    // Note that `m_LSO` is sorted at the beginning of encoding.
    if( sig.first ) {
        auto itr = std::lower_bound( m_LOS.begin(), m_LOS.end(), sig.second,
                   [](auto& otl, auto& val){return otl.location < val;} );
        assert( itr != m_LOS.end() );
        assert( (*itr).location == sig.second );  // Must find exactly this index
        sig.second = std::distance( m_LOS.begin(), itr );
    }

    return sig;
}

auto speck::SPERR::m_process_S_encoding(size_t  idx1,    size_t idx2,
                                        size_t& counter, bool   output) -> bool
{
    auto& set     = m_LIS[idx1][idx2];
    auto  sig_rtn = m_decide_significance(set);
    bool  is_sig  = sig_rtn.first;
    auto  sig_idx = sig_rtn.second;

    if( output )
        m_bit_buffer.push_back( is_sig );

    // Sanity check: when `output` is false, `is_sig` must be true.
    assert( output || is_sig );

    if (is_sig) {
        counter++;
        if (set.length == 1) { // Is a pixel
            m_bit_buffer.push_back(m_LOS[sig_idx].error >= 0.0); // Record its sign
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
    auto   sets    = m_part_set( m_LIS[idx1][idx2] );
    size_t counter = 0;

    // Process the 1st set
    const auto& s1 = sets[0];
    if (s1.length > 0) {
        auto newi1 = s1.part_level;
        m_LIS[newi1].emplace_back(s1);
        auto newi2 = m_LIS[newi1].size() - 1;
        if (m_encode_mode && m_process_S_encoding(newi1, newi2, counter, true))
            return true;
        else if (!m_encode_mode && m_process_S_decoding(newi1, newi2, counter, true))
            return true;
    }

    // Process the 2nd set
    const auto& s2 = sets[1];
    if( s2.length > 0 ) {
        auto newi1 = s2.part_level;
        m_LIS[newi1].emplace_back(s2);
        auto newi2 = m_LIS[newi1].size() - 1;

        // If the 1st set wasn't significant, then the 2nd set must be significant,
        // thus don't need to output/input this information.
        bool IO = (counter == 0 ? false : true);
        if (m_encode_mode && m_process_S_encoding(newi1, newi2, counter, IO) )
            return true;
        else if (!m_encode_mode && m_process_S_decoding(newi1, newi2, counter, IO))
            return true;
    }

    return false;
}

auto speck::SPERR::m_sorting_pass() -> bool
{
    size_t dummy = 0;
    for (size_t tmp = 1; tmp <= m_LIS.size(); tmp++) {
        size_t idx1 = m_LIS.size() - tmp;
        for (size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++) {
            if (m_encode_mode && m_process_S_encoding(idx1, idx2, dummy, true))
                return true;
            else if (!m_encode_mode && m_process_S_decoding(idx1, idx2, dummy, true))
                return true;
        }
    }

    return false;
}

auto speck::SPERR::m_refinement_pass_encoding() -> bool
{
    for (auto idx : m_LSP_old) {

        // First test if this pixel is still an outlier
        // Note that every refinement pass generates exactly 1 bit for each pixel in
        // `m_LSP_old` no matter if that pixel value is within error tolerance or not.
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

    // Now attach content from `m_LSP_new` to the end of `m_LSP_old`.
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

auto speck::SPERR::m_process_S_decoding(size_t  idx1,    size_t idx2,
                                        size_t& counter, bool   input) -> bool
{
    bool is_sig = true;
    if( input ) {
        is_sig = m_bit_buffer[m_bit_idx++];
        // Sanity check: the bit buffer should NOT be depleted at this point
        if( m_bit_idx == m_bit_buffer.size() )
            printf("m_bit_idx == %ld\n", m_bit_idx);
        assert(m_bit_idx < m_bit_buffer.size());
    }

    auto& set = m_LIS[idx1][idx2];

    if (is_sig) {
        counter++;
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

auto speck::SPERR::max_coeff_bits() const -> int32_t
{
    return m_max_coeff_bits;
}

auto speck::SPERR::get_encoded_bitstream() const -> smart_buffer_uint8
{
    // Header definition:
    // total_len  max_coeff_bits  num_of_bits
    // uint64_t   int32_t         uint64_t

    // Record the current (useful) number of bits
    const uint64_t num_bits = m_bit_buffer.size();

    // Create a copy of the current bit buffer with length being multiples of 8.
    // The purpose is to not mess up with the useful container.
    std::vector<bool> tmp_buf;
    tmp_buf.reserve( num_bits + 8 ); // No need to re-allocate when padding.
    tmp_buf = m_bit_buffer;
    while( tmp_buf.size() % 8 != 0 )
        tmp_buf.push_back(false);

    const size_t buf_len = m_header_size + tmp_buf.size() / 8;
    auto buf = std::make_unique<uint8_t[]>( buf_len );
    
    // Fill header
    size_t pos = 0;

    std::memcpy( buf.get(), &m_total_len, sizeof(m_total_len) );
    pos += sizeof(m_total_len);

    std::memcpy( buf.get() + pos, &m_max_coeff_bits, sizeof(m_max_coeff_bits) );
    pos += sizeof(m_max_coeff_bits);

    std::memcpy( buf.get() + pos, &num_bits, sizeof(num_bits) );
    pos += sizeof(num_bits);

    assert( pos == m_header_size );

    // Assemble the bitstream into bytes
    auto rtn = speck::pack_booleans( buf, tmp_buf, pos );
    if( rtn != RTNType::Good )
        return {nullptr, 0};
    else
        return {std::move(buf), buf_len};
}


auto speck::SPERR::parse_encoded_bitstream( const void* buf, size_t len ) -> RTNType
{
    // The buffer passed in is supposed to consist a header and then a compacted bitstream,
    // just like what was returned by `get_encoded_bitstream()`.
    // Note: header definition is documented in get_encoded_bitstream().

    const uint8_t* const ptr = static_cast<const uint8_t*>( buf );

    // Parse header
    uint64_t num_bits, pos = 0;

    std::memcpy( &m_total_len, ptr, sizeof(m_total_len) );
    pos += sizeof(m_total_len);

    std::memcpy( &m_max_coeff_bits, ptr + pos, sizeof(m_max_coeff_bits) );
    pos += sizeof(m_max_coeff_bits);

    std::memcpy( &num_bits, ptr + pos, sizeof(num_bits) );
    pos += sizeof(num_bits);

    assert( pos == m_header_size );

    // Unpack bits
    auto rtn = speck::unpack_booleans( m_bit_buffer, buf, len, pos );
    if( rtn != RTNType::Good )
        return rtn;

    // Restore the correct number of bits
    m_bit_buffer.resize( num_bits );

    return RTNType::Good;
}

auto speck::SPERR::get_sperr_stream_size( const void* buf ) const -> uint64_t
{
    // Given the header definition in `get_encoded_bitstream()`, directly
    // go retrieve the value stored in byte 12-20.
    const uint8_t* const ptr = static_cast<const uint8_t*>( buf );
    uint64_t num_bits;
    std::memcpy( &num_bits, ptr + 12, sizeof(num_bits) );
    while( num_bits % 8 != 0 )
        num_bits++;

    return (m_header_size + num_bits / 8);

}
