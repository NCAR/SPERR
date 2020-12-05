#include "SPECK3D.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>

#ifdef USE_OMP
    #include <omp.h>
#endif

//
// Class SPECKSet3D
//
auto speck::SPECKSet3D::is_pixel() const -> bool
{
    return (length_x == 1 && length_y == 1 && length_z == 1);
}

auto speck::SPECKSet3D::is_empty() const -> bool
{
    return (length_z == 0 || length_y == 0 || length_x == 0);
}


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

void speck::SPECK3D::get_dims( size_t& x, size_t& y, size_t& z ) const
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

#ifdef QZ_TERM
    void speck::SPECK3D::set_quantization_term_level( int32_t lev )
    {
        m_qz_term_lev   = lev;
    }

#endif


void speck::SPECK3D::m_clean_LIS()
{
    // This loop is in the order of one or two dozens, so not bother parallel here.
    for( size_t i = 0; i < m_LIS.size(); i++ ) {
        auto it = std::remove_if( m_LIS[i].begin(), m_LIS[i].end(),
                  [](const auto& s) { return s.type == SetType::Garbage; });
        m_LIS[i].erase( it, m_LIS[i].end() );
    }

    // Let's also clean up m_LIP.
    auto it = std::remove( m_LIP.begin(), m_LIP.end(), m_u64_garbage_val );
    m_LIP.erase( it, m_LIP.end() );
}

auto speck::SPECK3D::encode() -> RTNType
{
    if( m_ready_to_encode() == false )
        return RTNType::Error;
    m_encode_mode = true;

    m_initialize_sets_lists();

    m_bit_buffer.clear();
    m_bit_buffer.reserve(m_budget);
    auto max_coeff = speck::make_coeff_positive(m_coeff_buf, m_coeff_len, m_sign_array);

    // When max_coeff is between 0.0 and 1.0, std::log2(max_coeff) will become a
    // negative value. std::floor() will always find the smaller integer value,
    // which will always reconstruct to a bitplane value that is smaller than max_coeff. 
    // Also, when max_coeff is close to 0.0, std::log2(max_coeff) can
    // have a pretty big magnitude, so we use int32_t here.
    //
    m_max_coeff_bits = int32_t(std::floor(std::log2(max_coeff)));
    m_threshold      = std::pow(2.0, double(m_max_coeff_bits));

#ifdef QZ_TERM
    // If the requested termination level is already above max_coeff_bits, directly return. 
    if( m_qz_term_lev > m_max_coeff_bits )
        return RTNType::InvalidParam;

    int32_t current_qz_level = m_max_coeff_bits;
#endif

    // We say that we run 128 iterations at most.
    for( int iteration = 0; iteration < 128; iteration++ ) {

        // Valid threshold is between 0.0 and 1.0.
        //   Smaller thresholds result in more memory traverse and less random access.
        //   Bigger thresholds result in more memory random access and less traverse.
        //   Servers can use 0.9 or above; laptops can use 0.6 or below.
        //   (No one-size-fit-all)
        const float threshold = 0.9;
        if( m_LSP_old.size() > size_t(m_coeff_len * threshold) ) {
            m_sig_map.assign( m_coeff_len, false );
            for( size_t i = 0; i < m_coeff_len; i++ ) {
                if( m_coeff_buf[i] >= m_threshold )
                    m_sig_map[i] = true;
            }
            m_sig_map_enabled = true;
        }
        else {
            m_sig_map_enabled = false;
        }

#ifdef QZ_TERM
        // The actual encoding steps
        // Note that in QZ_TERM mode, only check termination at the end of bitplanes.
        m_sorting_pass_encode();
        m_refinement_pass_encode();
        
        // Let's test if we need to terminate
        if ( current_qz_level <= m_qz_term_lev )
            break;
        current_qz_level--;
#else
        // The following two functions only return `BitBudgetMet` or `Good`.
        if (m_sorting_pass_encode() == RTNType::BitBudgetMet )
            break;

        if (m_refinement_pass_encode() == RTNType::BitBudgetMet )
            break;
#endif

        m_threshold *= 0.5;
        m_clean_LIS();
    }

#ifdef QZ_TERM
    // If the bit buffer has the last byte empty, let's fill in zero's.
    while( m_bit_buffer.size() % 8 != 0 )
        m_bit_buffer.push_back( false );
#endif

    return RTNType::Good;
}

auto speck::SPECK3D::decode() -> RTNType
{
    if( m_ready_to_decode() == false )
        return RTNType::Error;
    m_encode_mode = false;

    // By default, decode all the available bits
    if (m_budget == 0 || m_budget > m_bit_buffer.size() )
        m_budget = m_bit_buffer.size();

    // initialize coefficients to be zero, and sign array to be all positive
    m_coeff_buf = speck::unique_malloc<double>(m_coeff_len);
    auto begin  = speck::uptr2itr( m_coeff_buf );
    auto end    = speck::uptr2itr( m_coeff_buf, m_coeff_len );
    std::fill( begin, end, 0.0 );
    m_sign_array.assign(m_coeff_len, true);

    m_initialize_sets_lists();

    m_bit_idx   = 0;
    m_threshold = std::pow(2.0, float(m_max_coeff_bits));
    for (size_t bitplane = 0; bitplane < 128; bitplane++) {

        // The following 2 methods always return `BitBudgetMet` or `Good`.
        auto rtn = m_sorting_pass_decode();
        if( rtn == RTNType::BitBudgetMet)
            break;
        assert( rtn == RTNType::Good );

        rtn = m_refinement_pass_decode();
        if( rtn == RTNType::BitBudgetMet)
            break;
        assert( rtn == RTNType::Good );

        m_threshold *= 0.5;

        m_clean_LIS();
    }

    // If the loop above aborted before all newly significant pixels are initialized,
    // we finish them here!
    for( auto idx : m_LSP_new )
        m_coeff_buf[idx] = m_threshold * 1.5;

    // Restore coefficient signs by setting some of them negative
    for (size_t i = 0; i < m_sign_array.size(); i++) {
        if (!m_sign_array[i])
            m_coeff_buf[i] = -m_coeff_buf[i];
    }

    return RTNType::Good;
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
    m_LIP.clear();

    // Starting from a set representing the whole volume, identify the smaller sets
    //   and put them in LIS accordingly.
    SPECKSet3D big;
    big.length_x = uint32_t(m_dim_x); // Truncate 64-bit int to 32-bit, but should be OK.
    big.length_y = uint32_t(m_dim_y); // Truncate 64-bit int to 32-bit, but should be OK.
    big.length_z = uint32_t(m_dim_z); // Truncate 64-bit int to 32-bit, but should be OK.

    const auto num_of_xforms_xy = speck::num_of_xforms(std::min(m_dim_x, m_dim_y));
    const auto num_of_xforms_z  = speck::num_of_xforms(m_dim_z);
    size_t     xf               = 0;

    while (xf < num_of_xforms_xy && xf < num_of_xforms_z) {
        auto subsets = m_partition_S_XYZ(big);
        big = subsets[0];
        // Iterate the rest subsets.
        for (size_t i = 1; i < subsets.size(); i++) {
            const auto parts = subsets[i].part_level;
            m_LIS[parts].emplace_back(subsets[i]);
        }
        xf++;
    }

    // One of these two conditions could happen if num_of_xforms_xy != num_of_xforms_z
    if (xf < num_of_xforms_xy) {
        while (xf < num_of_xforms_xy) {
            auto subsets = m_partition_S_XY(big);
            big = subsets[0];
            // Iterate the rest subsets.
            for (size_t i = 1; i < subsets.size(); i++) {
                const auto parts = subsets[i].part_level;
                m_LIS[parts].emplace_back(subsets[i]);
            }
            xf++;
        }
    } else if (xf < num_of_xforms_z) {
        while (xf < num_of_xforms_z) {
            auto subsets = m_partition_S_Z(big);
            big              = subsets[0];
            const auto parts = subsets[1].part_level;
            m_LIS[parts].emplace_back(subsets[1]);
            xf++;
        }
    }

    // Right now big is the set that's most likely to be significant, so insert
    // it at the front of it's corresponding vector. One-time expense.
    const auto parts = big.part_level;
    m_LIS[parts].insert(m_LIS[parts].begin(), big);

    // Initialize LSP.
    // Note that `m_LSP_old` usually grow close to the full length, so we reserve space now.
    m_LSP_new.clear();
    m_LSP_old.clear();
    m_LSP_old.reserve( m_coeff_len );
}

auto speck::SPECK3D::m_sorting_pass_encode() -> RTNType
{
    // Since we have a separate representation of LIP, let's process that list first!
    // Note that the code here to process m_LIP is equivalent to m_process_P_encode(),
    //   but re-organized in a way that's more friendly to OpenMP parallelization.
    // Also note that in `fixed size` compression mode, this code will result in more
    //   computation than necessary, namely when the bit budget is met in the middle of 
    //   processing m_LIP. However, we consider `fixed QZ_TERM` is a more common use case
    //   thus warrents this design decision.

    // In addition to the pre-defined `m_false`, `m_true`, and `m_discard`,
    //   let's define two more states that augments `m_true`:
    const uint8_t sig_pos = 3;  // this pixel is significant and has a positive sign
    const uint8_t sig_neg = 4;  // this pixel is significant and has a negative sign

    // `m_LSP_new` is now empty from the previous iteration, and ready to receive input from `m_LIP`.
    m_LSP_new.assign( m_LIP.size(), m_u64_garbage_val );
    m_tmp_result.assign( m_LIP.size(), m_discard );
    //
    // Experiments show that though we use a temporary storage space and that
    //   this omp section is rather simple, it's still faster than serial execution with
    //   direct push to `m_bit_buffer`. Also, this code isn't slower even without OMP.
    //   I guess the random access of `m_coeff_buf` and `m_sign_array` are just benefiting
    //   from concurrent queries a lot.
    //

    if( m_sig_map_enabled ) {
        #pragma omp parallel for
        for( size_t i = 0; i < m_LIP.size(); i++ ) {
            const auto pixel_idx = m_LIP[i];
            if( m_sig_map[ pixel_idx ] ) {
                m_tmp_result[i] = m_sign_array[pixel_idx] ? sig_pos : sig_neg;
                m_LSP_new[i]    = pixel_idx;
                m_LIP[i]        = m_u64_garbage_val;
            }
            else
                m_tmp_result[i] = m_false;
        }
    }
    else {
        #pragma omp parallel for
        for (size_t i = 0; i < m_LIP.size(); i++) {
            const auto pixel_idx = m_LIP[i];
            if( m_coeff_buf[ pixel_idx ] >= m_threshold ) {
                m_tmp_result[i] = m_sign_array[pixel_idx] ? sig_pos : sig_neg;
                m_LSP_new[i]    = pixel_idx;
                m_LIP[i]        = m_u64_garbage_val;
            }
            else
                m_tmp_result[i] = m_false;
        }
    }

    auto end_itr = std::remove( m_LSP_new.begin(), m_LSP_new.end(), m_u64_garbage_val );
    m_LSP_new.erase( end_itr, m_LSP_new.end() );

    // Now we put `m_tmp_result` into `m_bit_buffer` in serial.
    //
    for( size_t i = 0; i < m_tmp_result.size(); i++ ) {
        const auto e = m_tmp_result[i];
        if( e == sig_pos ) {
#ifdef QZ_TERM
            m_bit_buffer.push_back( true ); // this pixel is significant
            m_bit_buffer.push_back( true ); // this pixel has a positive sign
#else
            m_bit_buffer.push_back( true );
            if( m_bit_buffer.size() >= m_budget )
                return RTNType::BitBudgetMet;
            m_bit_buffer.push_back( true );
            if( m_bit_buffer.size() >= m_budget )
                return RTNType::BitBudgetMet;
#endif
        }
        else if( e == sig_neg ) {
#ifdef QZ_TERM
            m_bit_buffer.push_back( true );  // this pixel is significant
            m_bit_buffer.push_back( false ); // this pixel has a negative sign
#else
            m_bit_buffer.push_back( true );
            if( m_bit_buffer.size() >= m_budget )
                return RTNType::BitBudgetMet;
            m_bit_buffer.push_back( false );
            if( m_bit_buffer.size() >= m_budget )
                return RTNType::BitBudgetMet;
#endif
        }
        else if( e == m_false ) {
            m_bit_buffer.push_back( false );
#ifndef QZ_TERM
            if( m_bit_buffer.size() >= m_budget )
                return RTNType::BitBudgetMet;
#endif
        }
    }

    // Then we process regular sets in LIS.
    //
    for (size_t tmp = 1; tmp <= m_LIS.size(); tmp++) {
        // From the end of m_LIS to its front
        size_t idx1 = m_LIS.size() - tmp;
        for (size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++) {
            const auto& s = m_LIS[idx1][idx2];
            assert(s.type != SetType::Garbage);

#ifdef QZ_TERM
            m_process_S_encode(idx1, idx2);
#else
            auto rtn = m_process_S_encode(idx1, idx2);
            if( rtn == RTNType::BitBudgetMet )
                return rtn;
            assert( rtn == RTNType::Good );
#endif

        }
    }

    return RTNType::Good;
}

auto speck::SPECK3D::m_sorting_pass_decode() -> RTNType
{
    // Since we have a separate representation of LIP, let's process that list first!
    //
    // Note that a large portion of the content in `m_LIP` will go to `m_LSP_new`,
    //   and `m_LSP_new` is empty at this point, so cheapest to re-allocate right now!
    if( m_LSP_new.capacity() < m_LIP.size() ) {
        m_LSP_new.reserve( std::max( m_LSP_new.capacity() * 2, m_LIP.size() ) );
    }
    for (size_t i = 0; i < m_LIP.size(); i++) {
        auto rtn = m_process_P_decode(i);
        if( rtn == RTNType::BitBudgetMet )
            return rtn;
        assert( rtn == RTNType::Good );
    }

    // Then we process regular sets in LIS.
    for (size_t tmp = 1; tmp <= m_LIS.size(); tmp++) {
        // From the end of m_LIS to its front
        size_t idx1 = m_LIS.size() - tmp;
        for (size_t idx2 = 0; idx2 < m_LIS[idx1].size(); idx2++) {
            const auto& s = m_LIS[idx1][idx2];
            auto rtn = m_process_S_decode(idx1, idx2);
            if( rtn == RTNType::BitBudgetMet )
                return rtn;
            assert( rtn == RTNType::Good );
        }
    }

    return RTNType::Good;
}


auto speck::SPECK3D::m_refinement_pass_encode() -> RTNType
{
    // First process `m_LSP_old`.
    //
    m_tmp_result.assign( m_LSP_old.size(), m_false );

    if( m_sig_map_enabled ) {
        #pragma omp parallel for
        for( size_t i = 0; i < m_LSP_old.size(); i++ ) {
            if( m_sig_map[ m_LSP_old[i] ] )
                m_tmp_result[i] = m_true;
        }
    }
    else {
        #pragma omp parallel for
        for (size_t i = 0; i < m_LSP_old.size(); i++) {
            const auto pos = m_LSP_old[i];
            if (m_coeff_buf[pos] >= m_threshold) {
                m_coeff_buf[pos] -= m_threshold;
                m_tmp_result[i]   = m_true;
            }
        }
    }

    // Now attach the true/false outputs from `m_tmp_result` to `m_bit_buffer` 
    //
    for( auto result : m_tmp_result ) {
        m_bit_buffer.push_back( result != m_false );
#ifndef QZ_TERM
        if( m_bit_buffer.size() >= m_budget ) 
            return RTNType::BitBudgetMet;
#endif
    }

    // Second, process `m_LSP_new`
    //
    if( m_sig_map_enabled ) {
        #pragma omp parallel for
        for( size_t i = 0; i < m_coeff_len; i++ ) {
            if( m_coeff_buf[i] >= m_threshold )
                m_coeff_buf[i] -= m_threshold;
        }
    }
    else {
        #pragma omp parallel for
        for( size_t i = 0; i < m_LSP_new.size(); i++ )
            m_coeff_buf[ m_LSP_new[i] ] -= m_threshold;
    }

    // Third, attached `m_LSP_new` to the end of `m_LSP_old`.
    //
    m_LSP_old.insert( m_LSP_old.end(), m_LSP_new.cbegin(), m_LSP_new.cend() );

    // Fourth, clear `m_LSP_new`.
    //
    m_LSP_new.clear();

    return RTNType::Good;
}


auto speck::SPECK3D::m_refinement_pass_decode() -> RTNType
{
    // First, process `m_LSP_old`
    //
    const size_t num_bits   = std::min( m_budget - m_bit_idx, m_LSP_old.size() );
    const double half_T     = m_threshold * 0.5;
    const double neg_half_T = m_threshold * -0.5;

    #pragma omp parallel for
    for( size_t i = 0; i < num_bits; i++ ) {
        m_coeff_buf[ m_LSP_old[i] ] += m_bit_buffer[ m_bit_idx + i ] ? 
                                       half_T : neg_half_T;
    }
    m_bit_idx += num_bits;
    if (m_bit_idx >= m_budget)
        return RTNType::BitBudgetMet;
    
    // Second, process `m_LSP_new`
    //
    const double one_half_T = m_threshold * 1.5;
    #pragma omp parallel for
    for( size_t i = 0; i < m_LSP_new.size(); i++ ) {
        m_coeff_buf[ m_LSP_new[i] ] = one_half_T;
    }

    // Third, attached `m_LSP_new` to the end of `m_LSP_old`.
    //
    m_LSP_old.insert( m_LSP_old.end(), m_LSP_new.cbegin(), m_LSP_new.cend() );

    // Fourth, clear `m_LSP_new`.
    //
    m_LSP_new.clear();

    return RTNType::Good;
}

auto speck::SPECK3D::m_process_P_encode(size_t loc) -> RTNType
{
    const auto pixel_idx = m_LIP[loc];

    // Decide the significance of this pixel
    const bool this_pixel_is_sig = (m_coeff_buf[pixel_idx] >= m_threshold);
    m_bit_buffer.push_back(this_pixel_is_sig);

#ifndef QZ_TERM
    if (m_bit_buffer.size() >= m_budget) 
        return RTNType::BitBudgetMet;
#endif

    if (this_pixel_is_sig) {
        // Output pixel sign
        m_bit_buffer.push_back(m_sign_array[pixel_idx]);
        m_LSP_new.push_back( pixel_idx );
        m_LIP[loc] = m_u64_garbage_val;

#ifndef QZ_TERM
        if (m_bit_buffer.size() >= m_budget)
            return RTNType::BitBudgetMet;
#endif
    }

    return RTNType::Good;
}

auto speck::SPECK3D::m_process_S_encode(size_t idx1, size_t idx2) -> RTNType 
{
    auto& set = m_LIS[idx1][idx2];

    // decide the significance of this set
    set.signif              = Significance::Insig;
    const size_t slice_size = m_dim_x * m_dim_y;

    // Also, this old-fashioned serial implementation is faster than using OMP
    //   at any level (Z, Y, X) of this loop.
    if( m_sig_map_enabled ) {
        for (auto z = set.start_z; z < (set.start_z + set.length_z); z++) {
            const size_t slice_offset = z * slice_size;
            for (auto y = set.start_y; y < (set.start_y + set.length_y); y++) {
                const size_t col_offset = slice_offset + y * m_dim_x;
                for( auto x = set.start_x; x < (set.start_x + set.length_x); x++ ) {
                    if( m_sig_map[ col_offset + x ] ) {
                        set.signif = Significance::Sig;
                        goto end_loop_label;
                    }
                }   
            }
        }
    }
    else {
        for (auto z = set.start_z; z < (set.start_z + set.length_z); z++) {
            const size_t slice_offset = z * slice_size;
            for (auto y = set.start_y; y < (set.start_y + set.length_y); y++) {
                const size_t col_offset = slice_offset + y * m_dim_x;
                auto begin = speck::uptr2itr( m_coeff_buf, col_offset + set.start_x );
                auto end   = begin + set.length_x;
                if( std::any_of( begin, end, [tmp = m_threshold](auto& val){return val >= tmp;}) ) {
                    set.signif = Significance::Sig;
                    goto end_loop_label;
                }
            }
        }
    }
end_loop_label:
    m_bit_buffer.push_back(set.signif != Significance::Insig); // output true/false

#ifndef QZ_TERM
    if (m_bit_buffer.size() >= m_budget)
        return RTNType::BitBudgetMet;
#endif

    if (set.signif == Significance::Sig) {
#ifdef QZ_TERM
        m_code_S(idx1, idx2);
#else
        auto rtn = m_code_S(idx1, idx2);
        if( rtn == RTNType::BitBudgetMet )
            return RTNType::BitBudgetMet;
        assert( rtn == RTNType::Good );
#endif
        set.type = SetType::Garbage; // this current set is gonna be discarded.
    }

    return RTNType::Good;
}

auto speck::SPECK3D::m_process_P_decode(size_t loc) -> RTNType
{
    // When decoding, check bit budget before attempting to read a bit
    if (m_bit_idx >= m_budget )
        return RTNType::BitBudgetMet;
    if( m_bit_buffer[m_bit_idx++] ) {   // If this pixel is significant
        const auto pixel_idx = m_LIP[loc];

        // When decoding, check bit budget before attempting to read a bit
        if (m_bit_idx >= m_budget )
            return RTNType::BitBudgetMet;
        if( !m_bit_buffer[m_bit_idx++] )
            m_sign_array[pixel_idx] = false;

        // This pixel is moved to `m_LSP_new` from `m_LIP`.
        m_LIP[loc] = m_u64_garbage_val;
        m_LSP_new.push_back( pixel_idx );
    }

    return RTNType::Good;
}

auto speck::SPECK3D::m_process_S_decode(size_t idx1, size_t idx2) -> RTNType
{
    assert(!m_LIS[idx1][idx2].is_pixel());

    if (m_bit_idx >= m_budget)
        return RTNType::BitBudgetMet;

    auto& set  = m_LIS[idx1][idx2];
    set.signif = m_bit_buffer[m_bit_idx++] ? Significance::Sig : Significance::Insig;
    int rtn    = 0;

    if (set.signif == Significance::Sig) {
        auto rtn = m_code_S(idx1, idx2);
        if( rtn == RTNType::BitBudgetMet )
            return RTNType::BitBudgetMet;
        assert( rtn == RTNType::Good );

        set.type = SetType::Garbage; // this current set is gonna be discarded.
    }

    return RTNType::Good;
}

auto speck::SPECK3D::m_code_S(size_t idx1, size_t idx2) -> RTNType
{
    const auto& set     = m_LIS[idx1][idx2];
    const auto& subsets = m_partition_S_XYZ(set);

    for (const auto& s : subsets) {
        if (s.is_pixel()) {
            m_LIP.push_back(s.start_z * m_dim_x * m_dim_y + s.start_y * m_dim_x + s.start_x);

            if (m_encode_mode) {

#ifdef QZ_TERM
                m_process_P_encode(m_LIP.size() - 1);
#else
                auto rtn = m_process_P_encode(m_LIP.size() - 1);
                if( rtn == RTNType::BitBudgetMet )
                    return RTNType::BitBudgetMet;
                assert( rtn == RTNType::Good );
#endif

            } else {    // decoding mode
                auto rtn = m_process_P_decode(m_LIP.size() - 1);
                if( rtn == RTNType::BitBudgetMet )
                    return RTNType::BitBudgetMet;
                assert( rtn == RTNType::Good );
            }
        } else if (!s.is_empty()) {
            const auto newidx1 = s.part_level;
            m_LIS[newidx1].emplace_back(s);
            const auto newidx2 = m_LIS[newidx1].size() - 1;

            if (m_encode_mode) {

#ifdef QZ_TERM
                m_process_S_encode(newidx1, newidx2);
#else
                auto rtn = m_process_S_encode(newidx1, newidx2);
                if( rtn == RTNType::BitBudgetMet )
                    return RTNType::BitBudgetMet;
                assert( rtn == RTNType::Good );
#endif

            } else {    // decoding mode
                auto rtn = m_process_S_decode(newidx1, newidx2);
                if( rtn == RTNType::BitBudgetMet )
                    return RTNType::BitBudgetMet;
                assert( rtn == RTNType::Good );
            }
        }
    }

    return RTNType::Good;
}

auto speck::SPECK3D::m_ready_to_encode() const -> bool
{
    if (m_coeff_buf == nullptr)
        return false;
    if (m_dim_x == 0 || m_dim_y == 0 || m_dim_z == 0)
        return false;

#ifndef QZ_TERM
    if (m_budget == 0 || m_budget > m_coeff_len * 8)
        return false;
#endif

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


auto speck::SPECK3D::get_encoded_bitstream() const -> std::pair<buffer_type_uint8, size_t>
{
    // Header definition:
    // information: dim_x,     dim_y,     dim_z,     image_mean,  max_coeff_bits,  bitstream
    // format:      uint32_t,  uint32_t,  uint32_t,  double       int32_t,         packed_bytes
    const size_t header_size = 24;
    uint32_t dims[3] { uint32_t(m_dim_x), uint32_t(m_dim_y), uint32_t(m_dim_z) };

    // Create and fill header buffer
    uint8_t header[header_size];

    size_t pos = 0;
    std::memcpy(header, dims, sizeof(dims));
    pos += sizeof(dims);
    std::memcpy(header + pos, &m_image_mean, sizeof(m_image_mean));
    pos += sizeof(m_image_mean);
    std::memcpy(header + pos, &m_max_coeff_bits, sizeof(m_max_coeff_bits));
    pos += sizeof(m_max_coeff_bits);
    assert(pos == header_size);

    return m_assemble_encoded_bitstream( header, header_size );
}


auto speck::SPECK3D::parse_encoded_bitstream( const void* comp_buf, size_t comp_size) -> RTNType
{
    // See method get_encoded_bitstream() for header definitions
    const size_t header_size = 24;
    uint8_t header[ header_size ];

    // break down the compressed buffer
    auto rtn = m_disassemble_encoded_bitstream( header, header_size, comp_buf, comp_size );
    if( rtn != RTNType::Good )
        return rtn;

    // Parse the header
    uint32_t dims[3] = {0, 0, 1};
    size_t   pos = 0;
    std::memcpy(dims, header, sizeof(dims));
    pos += sizeof(dims);
    std::memcpy(&m_image_mean, header + pos, sizeof(m_image_mean));
    pos += sizeof(m_image_mean);
    std::memcpy(&m_max_coeff_bits, header + pos, sizeof(m_max_coeff_bits));
    pos += sizeof(m_max_coeff_bits);
    assert(pos == header_size);

    // If dims[2] is 1, then this file is for 2D planes! We abort immediately.
    if( dims[2] == 1 )
        return RTNType::DimMismatch;

    this->set_dims(size_t(dims[0]), size_t(dims[1]), size_t(dims[2]));

    return RTNType::Good;
}


auto speck::SPECK3D::m_partition_S_XYZ(const SPECKSet3D& set) const -> std::array<SPECKSet3D, 8>
{
    std::array<SPECKSet3D, 8> subsets;

    const uint32_t split_x[2] { set.length_x - set.length_x / 2, set.length_x / 2 };
    const uint32_t split_y[2] { set.length_y - set.length_y / 2, set.length_y / 2 };
    const uint32_t split_z[2] { set.length_z - set.length_z / 2, set.length_z / 2 };

    for (auto& s : subsets) {
        s.part_level = set.part_level;
        if (split_x[1] > 0)
            s.part_level++;
        if (split_y[1] > 0)
            s.part_level++;
        if (split_z[1] > 0)
            s.part_level++;
    }

    //
    // The actual figuring out where it starts/ends part...
    //
    const size_t offsets[3] { 1, 2, 4 };
    // subset (0, 0, 0)
    size_t sub_i  = 0 * offsets[0] + 0 * offsets[1] + 0 * offsets[2];
    auto&  sub0   = subsets[sub_i];
    sub0.start_x  = set.start_x;
    sub0.length_x = split_x[0];
    sub0.start_y  = set.start_y;
    sub0.length_y = split_y[0];
    sub0.start_z  = set.start_z;
    sub0.length_z = split_z[0];

    // subset (1, 0, 0)
    sub_i         = 1 * offsets[0] + 0 * offsets[1] + 0 * offsets[2];
    auto& sub1    = subsets[sub_i];
    sub1.start_x  = set.start_x + split_x[0];
    sub1.length_x = split_x[1];
    sub1.start_y  = set.start_y;
    sub1.length_y = split_y[0];
    sub1.start_z  = set.start_z;
    sub1.length_z = split_z[0];

    // subset (0, 1, 0)
    sub_i         = 0 * offsets[0] + 1 * offsets[1] + 0 * offsets[2];
    auto& sub2    = subsets[sub_i];
    sub2.start_x  = set.start_x;
    sub2.length_x = split_x[0];
    sub2.start_y  = set.start_y + split_y[0];
    sub2.length_y = split_y[1];
    sub2.start_z  = set.start_z;
    sub2.length_z = split_z[0];

    // subset (1, 1, 0)
    sub_i         = 1 * offsets[0] + 1 * offsets[1] + 0 * offsets[2];
    auto& sub3    = subsets[sub_i];
    sub3.start_x  = set.start_x + split_x[0];
    sub3.length_x = split_x[1];
    sub3.start_y  = set.start_y + split_y[0];
    sub3.length_y = split_y[1];
    sub3.start_z  = set.start_z;
    sub3.length_z = split_z[0];

    // subset (0, 0, 1)
    sub_i         = 0 * offsets[0] + 0 * offsets[1] + 1 * offsets[2];
    auto& sub4    = subsets[sub_i];
    sub4.start_x  = set.start_x;
    sub4.length_x = split_x[0];
    sub4.start_y  = set.start_y;
    sub4.length_y = split_y[0];
    sub4.start_z  = set.start_z + split_z[0];
    sub4.length_z = split_z[1];

    // subset (1, 0, 1)
    sub_i         = 1 * offsets[0] + 0 * offsets[1] + 1 * offsets[2];
    auto& sub5    = subsets[sub_i];
    sub5.start_x  = set.start_x + split_x[0];
    sub5.length_x = split_x[1];
    sub5.start_y  = set.start_y;
    sub5.length_y = split_y[0];
    sub5.start_z  = set.start_z + split_z[0];
    sub5.length_z = split_z[1];

    // subset (0, 1, 1)
    sub_i         = 0 * offsets[0] + 1 * offsets[1] + 1 * offsets[2];
    auto& sub6    = subsets[sub_i];
    sub6.start_x  = set.start_x;
    sub6.length_x = split_x[0];
    sub6.start_y  = set.start_y + split_y[0];
    sub6.length_y = split_y[1];
    sub6.start_z  = set.start_z + split_z[0];
    sub6.length_z = split_z[1];

    // subset (1, 1, 1)
    sub_i         = 1 * offsets[0] + 1 * offsets[1] + 1 * offsets[2];
    auto& sub7    = subsets[sub_i];
    sub7.start_x  = set.start_x + split_x[0];
    sub7.length_x = split_x[1];
    sub7.start_y  = set.start_y + split_y[0];
    sub7.length_y = split_y[1];
    sub7.start_z  = set.start_z + split_z[0];
    sub7.length_z = split_z[1];

    return subsets;
}

auto speck::SPECK3D::m_partition_S_XY(const SPECKSet3D& set) const -> std::array<SPECKSet3D, 4>
{
    std::array<SPECKSet3D, 4> subsets;

    const uint32_t split_x[2] { set.length_x - set.length_x / 2, set.length_x / 2 };
    const uint32_t split_y[2] { set.length_y - set.length_y / 2, set.length_y / 2 };

    for (auto& s : subsets) {
        s.part_level = set.part_level;
        if (split_x[1] > 0)
            s.part_level++;
        if (split_y[1] > 0)
            s.part_level++;
    }

    //
    // The actual figuring out where it starts/ends part...
    //
    const size_t offsets[3] { 1, 2, 4 };
    // subset (0, 0, 0)
    size_t sub_i  = 0 * offsets[0] + 0 * offsets[1] + 0 * offsets[2];
    auto&  sub0   = subsets[sub_i];
    sub0.start_x  = set.start_x;
    sub0.length_x = split_x[0];
    sub0.start_y  = set.start_y;
    sub0.length_y = split_y[0];
    sub0.start_z  = set.start_z;
    sub0.length_z = set.length_z;

    // subset (1, 0, 0)
    sub_i         = 1 * offsets[0] + 0 * offsets[1] + 0 * offsets[2];
    auto& sub1    = subsets[sub_i];
    sub1.start_x  = set.start_x + split_x[0];
    sub1.length_x = split_x[1];
    sub1.start_y  = set.start_y;
    sub1.length_y = split_y[0];
    sub1.start_z  = set.start_z;
    sub1.length_z = set.length_z;

    // subset (0, 1, 0)
    sub_i         = 0 * offsets[0] + 1 * offsets[1] + 0 * offsets[2];
    auto& sub2    = subsets[sub_i];
    sub2.start_x  = set.start_x;
    sub2.length_x = split_x[0];
    sub2.start_y  = set.start_y + split_y[0];
    sub2.length_y = split_y[1];
    sub2.start_z  = set.start_z;
    sub2.length_z = set.length_z;

    // subset (1, 1, 0)
    sub_i         = 1 * offsets[0] + 1 * offsets[1] + 0 * offsets[2];
    auto& sub3    = subsets[sub_i];
    sub3.start_x  = set.start_x + split_x[0];
    sub3.length_x = split_x[1];
    sub3.start_y  = set.start_y + split_y[0];
    sub3.length_y = split_y[1];
    sub3.start_z  = set.start_z;
    sub3.length_z = set.length_z;

    return subsets;
}

auto speck::SPECK3D::m_partition_S_Z(const SPECKSet3D& set) const -> std::array<SPECKSet3D, 2>
{
    std::array<SPECKSet3D, 2> subsets;

    const uint32_t split_z[2] { set.length_z - set.length_z / 2, set.length_z / 2 };

    for (auto& s : subsets) {
        s.part_level = set.part_level;
        if (split_z[1] > 0)
            s.part_level++;
    }

    //
    // The actual figuring out where it starts/ends part...
    //
    // subset (0, 0, 0)
    auto& sub0    = subsets[0];
    sub0.start_x  = set.start_x;
    sub0.length_x = set.length_x;
    sub0.start_y  = set.start_y;
    sub0.length_y = set.length_y;
    sub0.start_z  = set.start_z;
    sub0.length_z = split_z[0];

    // subset (0, 0, 1)
    auto& sub1    = subsets[1];
    sub1.start_x  = set.start_x;
    sub1.length_x = set.length_x;
    sub1.start_y  = set.start_y;
    sub1.length_y = set.length_y;
    sub1.start_z  = set.start_z + split_z[0];
    sub1.length_z = split_z[1];

    return subsets;
}
