#include "SPECK2D.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <array>
#include <algorithm>


void speck::SPECK2D::take_coeffs( std::unique_ptr<double[]> ptr )
{
    m_coeff_buf = std::move( ptr );
}

template<typename T>
void speck::SPECK2D::copy_coeffs( const T* p )
{
    static_assert( std::is_floating_point<T>::value, 
                   "!! Only floating point values are supported !!" );
    assert( m_dim_x > 0 && m_dim_y > 0 );

    long num_of_vals = m_dim_x * m_dim_y;
    m_coeff_buf = std::make_unique<double[]>( num_of_vals );
    for( long i = 0; i < num_of_vals; i++ )
        m_coeff_buf[i] = p[i];
}
template void speck::SPECK2D::copy_coeffs( const float*  ); 
template void speck::SPECK2D::copy_coeffs( const double* );


void speck::SPECK2D::copy_bitstream( const std::vector<bool>& stream )
{
    m_bit_buffer = stream;
}


void speck::SPECK2D::take_bitstream( std::vector<bool>& stream )
{
    m_bit_buffer.resize( 0 );
    std::swap( m_bit_buffer, stream );
}


const std::vector<bool>& speck::SPECK2D::get_read_only_bitstream() const
{
    return m_bit_buffer;
}


std::vector<bool>& speck::SPECK2D::release_bitstream()
{
    return m_bit_buffer;
}


void speck::SPECK2D::assign_mean( double m )
{
    m_data_mean = m;
}


void speck::SPECK2D::assign_dims( long dx, long dy )
{
    m_dim_x = dx;
    m_dim_y = dy;
}


void speck::SPECK2D::assign_max_coeff_bits( uint16_t bits )
{
    m_max_coefficient_bits = bits;
}


uint16_t speck::SPECK2D::get_max_coeff_bits() const
{
    return m_max_coefficient_bits;
}


void speck::SPECK2D::assign_bit_budget( uint64_t budget )
{
    m_budget = budget;
}
    

int speck::SPECK2D::encode()
{
    assert( m_ready_to_encode() );
    m_encode_mode = true;

    m_initialize_sets_lists();

    // Get ready for the quantization loop!
    m_bit_buffer.clear();
    m_bit_buffer.reserve( m_budget + m_vec_init_capacity );
    auto max_coeff = speck::make_positive( m_coeff_buf.get(), m_dim_x * m_dim_y, m_sign_array );
    m_max_coefficient_bits = uint16_t( std::log2(max_coeff) );
    // ( when max_coeff is very close to zero, m_max_coefficient_bits could be zero.
    // I don't know how to deal with that situation yet... )
    assert( m_max_coefficient_bits > 0 );   
    m_threshold = std::pow( 2.0, double(m_max_coefficient_bits) );
    for( long bitplane = 0; bitplane < 2; bitplane++ )
    {
        if( m_sorting_pass() == 1 )
            return 1;
        if( m_refinement_pass() == 1 )
            return 1;

        m_threshold *= 0.5;

        m_clean_LIS();
    }

    return 0;
}


int speck::SPECK2D::decode()
{
    assert( m_ready_to_decode() );
    m_encode_mode = false;

#ifdef PRINT
    printf("\n\nstart decoding!\n");
#endif

    // initialize coefficients to be zero, and signs to be all positive
    long num_of_vals = m_dim_x * m_dim_y;
    m_coeff_buf = std::make_unique<double[]>( num_of_vals );
    for( long i = 0; i < num_of_vals; i++ )
        m_coeff_buf[i] = 0.0;
    m_sign_array.assign( num_of_vals, true );

    m_initialize_sets_lists();
    
    m_bit_idx = 0;
    m_threshold = std::pow( 2.0, double(m_max_coefficient_bits) );
    for( long bitplane = 0; bitplane < 128; bitplane++ )
    {
        if( m_sorting_pass() == 1 )
            return 1;
        if( m_refinement_pass() == 1 )
            return 1;

        m_threshold *= 0.5;

        m_clean_LIS();
    }

    return 0;
}


void speck::SPECK2D::m_initialize_sets_lists()
{
    auto num_of_parts  = m_num_of_partitions();
    auto num_of_xforms = speck::calc_num_of_xforms( std::min( m_dim_x, m_dim_y) );

    // prepare m_LIS
    m_LIS.clear();
    m_LIS.resize( num_of_parts + 1 );
    for( auto& v : m_LIS )  // Avoid frequent memory allocations.
        v.reserve( m_vec_init_capacity );
    m_LIS_garbage_cnt.assign( num_of_parts + 1, 0 );

    // prepare the root, S
    SPECKSet2D S( SPECKSetType::TypeS );
    S.part_level = num_of_xforms;
    m_calc_root_size( S );
    m_LIS[ S.part_level ].push_back( S );

    // clear m_LSP
    m_LSP.clear();
    m_LSP.reserve( m_vec_init_capacity );

    // prepare m_I
    m_I.part_level = num_of_xforms;
    m_I.start_x    = S.length_x;
    m_I.start_y    = S.length_y;
    m_I.length_x   = m_dim_x;
    m_I.length_y   = m_dim_y;
}
    
    
//
// Private methods
//
int speck::SPECK2D::m_sorting_pass( )
{
#ifdef PRINT
    printf("--> sorting pass, threshold = %f\n", m_threshold );
#endif

    if( m_encode_mode )
    {   // Update the significance map based on the current threshold
        speck::update_significance_map( m_coeff_buf.get(), m_dim_x * m_dim_y, 
                                        m_threshold, m_significance_map );
    }

    for( long idx1 = m_LIS.size() - 1; idx1 >= 0; idx1-- )
        for( long idx2  = 0; idx2 < m_LIS[idx1].size(); idx2++ )
        {
            auto& s = m_LIS[idx1][idx2];
            if( !s.garbage )
            {
                if( m_decide_set_significance( s )  == 1 )
                    return 1;
                if( m_process_S( idx1, idx2, true ) == 1 )
                    return 1;
            }
        }

    if( m_decide_set_significance( m_I ) )
        return 1;
    if( m_process_I() == 1 )
        return 1;

    return 0;
}


int speck::SPECK2D::m_refinement_pass( )
{
#ifdef PRINT
    printf("--> refinement pass, threshold = %f\n", m_threshold );
#endif
    for( auto& p : m_LSP )
    {
        if( p.signif == Significance::NewlySig )
            p.signif  = Significance::Sig;
        else
        {
            if( m_encode_mode )
            {
                if( m_output_refinement( p ) == 1 )
                    return 1;
            }
            else
            {
                if( m_input_refinement( p ) == 1 )
                    return 1;
            }
        }
    }

    return 0;
}


int speck::SPECK2D::m_process_S( long idx1, long idx2, bool code_this_set )
{
    auto& set = m_LIS[idx1][idx2];
    
#ifdef PRINT
    m_print_set( "process_S", set );

    if( (!m_encode_mode) && code_this_set )
    {
        auto bit = ( set.signif == Significance::Sig );
        std::string str = bit ? "s1" : "s0";
        std::cout << str << std::endl;
    }
#endif

    if( set.signif == Significance::Empty ) // Skip empty sets 
    {
        set.garbage = true;
        m_LIS_garbage_cnt[ set.part_level ]++;
        return 0;
    }

    if( m_encode_mode && code_this_set )
    {
        if( m_output_set_significance( set ) == 1 )
            return 1;
    }

    if( set.signif == Significance::Sig )
    {
        if( set.is_pixel() )
        {
            set.signif = Significance::NewlySig;
            if( m_encode_mode )
            {
                if( m_output_pixel_sign( set ) == 1 )
                    return 1;
            }
            else
            {
                if( m_input_pixel_sign( set ) == 1 )
                    return 1;
            }
            m_LSP.push_back( set ); // A copy is saved to m_LSP.
            set.garbage = true;     // This particular object will be discarded.
            m_LIS_garbage_cnt[ set.part_level ]++;
        }
        else
        {
            if( m_code_S( idx1, idx2 ) == 1 )
                return 1;
            set.garbage = true;     // This particular object will be discarded.
            m_LIS_garbage_cnt[ set.part_level ]++;
        }
    }

    return 0;
}


int speck::SPECK2D::m_code_S( long idx1, long idx2 )
{
    const auto& set = m_LIS[idx1][idx2];
    
#ifdef PRINT
    m_print_set( "code_S", set );
#endif

    std::array< SPECKSet2D, 4 > subsets;
    m_partition_S( set, subsets );

    // We count how many subsets are significant, and if the first 3 subsets ain't,
    // then the 4th one must be significant.
    long already_sig = 0;
    for( size_t i = 0; i < 3; i++ )
    {
        if( m_decide_set_significance( subsets[i] ) == 1 )
            return 1;
        if( subsets[i].signif == Significance::Sig )
            already_sig++;
    }
    if( already_sig == 0 )
        subsets[3].signif = Significance::Sig;
    else
    {
        if( m_decide_set_significance( subsets[3] ) == 1 )
            return 1;
    }

    // Definitely code the first 3 subsets
    bool code_set[4] = {true, true, true, true};
    if( already_sig == 0 )  // Might not need to code the 4th set.
        code_set[3] = false;

    for( size_t i = 0; i < subsets.size(); i++ )
    {
        auto& s = subsets[i];
        m_LIS[ s.part_level ].push_back( s );
        if( m_process_S( s.part_level, m_LIS[s.part_level].size() - 1, code_set[i] ) == 1 )
            return 1;
    }

    return 0;
}


void speck::SPECK2D::m_partition_S( const SPECKSet2D& set, std::array<SPECKSet2D, 4>& list ) const
{
    // The top-left set will have these bigger dimensions in case that 
    // the current set has odd dimensions.
    const auto detail_len_x = set.length_x / 2;
    const auto detail_len_y = set.length_y / 2;
    const auto approx_len_x = set.length_x - detail_len_x;
    const auto approx_len_y = set.length_y - detail_len_y;

    // Put generated subsets in the list the same order as did in QccPack.
    auto& BR      = list[0];               // Bottom right set
    BR.part_level = set.part_level + 1;
    BR.start_x    = set.start_x + approx_len_x;
    BR.start_y    = set.start_y + approx_len_y;
    BR.length_x   = detail_len_x;
    BR.length_y   = detail_len_y;

    auto& BL      = list[1];               // Bottom left set
    BL.part_level = set.part_level + 1;
    BL.start_x    = set.start_x;
    BL.start_y    = set.start_y + approx_len_y;
    BL.length_x   = approx_len_x;
    BL.length_y   = detail_len_y;

    auto& TR      = list[2];               // Top right set
    TR.part_level = set.part_level + 1;
    TR.start_x    = set.start_x + approx_len_x;
    TR.start_y    = set.start_y;
    TR.length_x   = detail_len_x;
    TR.length_y   = approx_len_y;

    auto& TL      = list[3];               // Top left set
    TL.part_level = set.part_level + 1;
    TL.start_x    = set.start_x;
    TL.start_y    = set.start_y;
    TL.length_x   = approx_len_x;
    TL.length_y   = approx_len_y;
}


int speck::SPECK2D::m_process_I()
{
    if( m_I.part_level == 0 )   // m_I is empty at this point
        return 0;

#ifdef PRINT
    m_print_set( "process_I", m_I );

    if( !m_encode_mode )
    {
        auto bit = ( m_I.signif == Significance::Sig );
        std::string str = bit ? "s1" : "s0";
        std::cout << str << std::endl;
    }
#endif
    
    if( m_encode_mode )
    {
        if( m_output_set_significance( m_I ) == 1 )
            return 1;
    }

    if( m_I.signif == Significance::Sig )
    {
        if( m_code_I() == 1 )
            return 1;
    }

    return 0;
}


int speck::SPECK2D::m_code_I()
{
    std::array< SPECKSet2D, 3 > subsets;
    m_partition_I( subsets );

    // We count how many subsets are significant, and if the first 2 subsets ain't,
    // then the 3rd one must be significant.
    long already_sig = 0;
    for( size_t i = 0; i < 2; i++ )
    {
        if( m_decide_set_significance( subsets[i] ) == 1 )
            return 1;
        if( subsets[i].signif == Significance::Sig )
            already_sig++;
    }
    if( already_sig == 0 )
        subsets[2].signif = Significance::Sig;
    else
    {
        if( m_decide_set_significance( subsets[2] ) == 1 )
            return 1;
    }

    // Definitely code the first 2 subsets
    bool code_set[3] = {true, true, true};
    if( already_sig == 0 )
        code_set[2] = false;

    for( size_t i = 0; i < subsets.size(); i++ )
    {
        auto& s = subsets[i];
        m_LIS[ s.part_level ].push_back( s );
        if( m_process_S( s.part_level, m_LIS[s.part_level].size() - 1, code_set[i] ) == 1 )
            return 1;
    }

    if( m_decide_set_significance( m_I ) == 1 )
        return 1;
    if( m_process_I() )
        return 1;

    return 0;
}


void speck::SPECK2D::m_partition_I( std::array<SPECKSet2D, 3>& subsets )
{
    const auto current_lev = m_I.part_level;
    long approx_len_x, detail_len_x;
    long approx_len_y, detail_len_y;
    speck::calc_approx_detail_len( m_dim_x, current_lev, approx_len_x, detail_len_x );
    speck::calc_approx_detail_len( m_dim_y, current_lev, approx_len_y, detail_len_y );

    // specify the subsets following the same order in QccPack
    auto& BR      = subsets[0];         // Bottom right
    BR.part_level = current_lev;
    BR.start_x    = approx_len_x;
    BR.start_y    = approx_len_y;
    BR.length_x   = detail_len_x;
    BR.length_y   = detail_len_y;

    auto& TR      = subsets[1];         // Top right
    TR.part_level = current_lev;
    TR.start_x    = approx_len_x;
    TR.start_y    = 0;
    TR.length_x   = detail_len_x;
    TR.length_y   = approx_len_y;

    auto& BL      = subsets[2];         // Bottom left
    BL.part_level = current_lev;
    BL.start_x    = 0;
    BL.start_y    = approx_len_y;
    BL.length_x   = approx_len_x;
    BL.length_y   = detail_len_y; 

    // Also update m_I
    m_I.part_level--;
    m_I.start_x += detail_len_x;
    m_I.start_y += detail_len_y;
}


int speck::SPECK2D::m_decide_set_significance( SPECKSet2D& set )
{
    // In case of an S set with zero dimension, mark it empty
    if( set.type() == SPECKSetType::TypeS && (set.length_x == 0 || set.length_y == 0) )
    {
        set.signif = Significance::Empty;
        return 0;
    }

    // If decoding, simply read a bit from the bitstream, no matter TypeS or TypeI.
    // Note: the only case this method returns 1 is when decoding and we have 
    //       already decoded enough number of bits.
    //       All other cases return 0, meaning successfully decided the 
    //       significance of this set.  
    if( !m_encode_mode )
    {
        if( m_bit_idx >= m_budget || m_bit_idx >= m_bit_buffer.size() )
            return 1;   // 1 means the algorithm needs to stop! 

        auto bit = m_bit_buffer[ m_bit_idx++ ];
        set.signif = bit ? Significance::Sig : Significance::Insig;
        return 0;
    }

    // If encoding, we start by marking it insignificant, and then compare with the
    // significance map.
    set.signif = Significance::Insig;

    // For TypeS sets, we test an obvious rectangle specified by this set.
    if( set.type() == SPECKSetType::TypeS )
    {
        for( long y = set.start_y; y < (set.start_y + set.length_y); y++ )
            for( long x = set.start_x; x < (set.start_x + set.length_x); x++ )
            {
                auto idx = y * m_dim_x + x;
                if( m_significance_map[ idx ] )
                {
                    set.signif = Significance::Sig;
                    return 0;
                }
            }
    }
    else    // For TypeI sets, we need to test two rectangles!
    {
        // First rectangle: directly to the right of the missing top-left corner
        for( long y = 0; y < set.start_y; y++ )
            for( long x = set.start_x; x < m_dim_x; x++ )
            {
                auto idx = y * m_dim_x + x;
                if( m_significance_map[ idx ] )
                {
                    set.signif = Significance::Sig;
                    return 0;
                }
            }

        // Second rectangle: the rest area at the bottom
        // Note: this rectangle is stored in a contiguous chunk of memory :)
        for( long i = set.start_y * m_dim_x; i < m_dim_x * m_dim_y; i++ )
        {
            if( m_significance_map[ i ] )
            {
                set.signif = Significance::Sig;
                return 0;
            }
        }
    }

    return 0;
}


// Output by printing it out
int speck::SPECK2D::m_output_set_significance( const SPECKSet2D& set )
{
#ifdef PRINT
    if( set.signif == Significance::Sig )
        std::cout << "s1" << std::endl;
    else
        std::cout << "s0" << std::endl;
#endif

    auto bit = (set.signif == Significance::Sig);
    m_bit_buffer.push_back( bit );
    
    // Let's also see if we're reached the bit budget
    if( m_bit_buffer.size() >= m_budget )
        return 1;
    else
        return 0;
}


// It outputs by printing out the value at this point.
int speck::SPECK2D::m_output_pixel_sign( const SPECKSet2D& pixel )
{
    auto idx = pixel.start_y * m_dim_x + pixel.start_x;

#ifdef PRINT
    if( m_sign_array[ idx ] )
        std::cout << "p1" << std::endl;
    else
        std::cout << "p0" << std::endl;
#endif

    m_bit_buffer.push_back( m_sign_array[idx] );

    // Progressive quantization!
    m_coeff_buf[ idx ] -= m_threshold;

    // Let's also see if we're reached the bit budget
    if( m_bit_buffer.size() >= m_budget )
        return 1;
    else
        return 0;
}


int speck::SPECK2D::m_input_pixel_sign( const SPECKSet2D& pixel )
{
    if( m_bit_idx >= m_budget || m_bit_idx >= m_bit_buffer.size() )
        return 1;

    auto idx = pixel.start_y * m_dim_x + pixel.start_x ;
    m_sign_array[ idx ] = m_bit_buffer[ m_bit_idx++ ];

    // Progressive quantization!
    m_coeff_buf[ idx ] = 1.5 * m_threshold;

#ifdef PRINT
    auto bit = m_sign_array[ idx ];
    std::string str = bit ? "p1" : "p0";
    std::cout << str << std::endl;
#endif

    return 0;
}


int speck::SPECK2D::m_output_refinement( const SPECKSet2D& pixel )
{
    auto idx = pixel.start_y * m_dim_x + pixel.start_x;

#ifdef PRINT
    if( m_coeff_buf[idx] >= m_threshold ) 
    {
        std::cout << "r1" << std::endl;
        m_coeff_buf[idx] -= m_threshold;
    }
    else
        std::cout << "r0" << std::endl;
#endif

    if( m_coeff_buf[idx] >= m_threshold ) 
    {
        m_bit_buffer.push_back( true );
        m_coeff_buf[idx] -= m_threshold;
    }
    else
        m_bit_buffer.push_back( false );

    // Let's also see if we're reached the bit budget
    if( m_bit_buffer.size() >= m_budget )
        return 1;
    else
        return 0;
}


int speck::SPECK2D::m_input_refinement( const SPECKSet2D& pixel )
{
    if( m_bit_idx >= m_budget || m_bit_idx >= m_bit_buffer.size() )
        return 1;

    auto bit = m_bit_buffer[ m_bit_idx++ ];
    auto idx = pixel.start_y * m_dim_x + pixel.start_x;
    m_coeff_buf[ idx ] += bit ? m_threshold * 0.5 : m_threshold * -0.5;

    return 0;
}

    
// Calculate the number of partitions able to be performed
long speck::SPECK2D::m_num_of_partitions() const
{
    long num_of_parts = 0;
    long dim_x = m_dim_x, dim_y = m_dim_y;
    while( dim_x > 1 || dim_y > 1 )
    {
        num_of_parts++;
        dim_x -= dim_x / 2;
        dim_y -= dim_y / 2;
    }
    return num_of_parts;
}



void speck::SPECK2D::m_calc_root_size( SPECKSet2D& root ) const
{
    long part_level = root.part_level;
    long low_len_x, high_len_x;
    long low_len_y, high_len_y;
    speck::calc_approx_detail_len( m_dim_x, part_level, low_len_x, high_len_x );
    speck::calc_approx_detail_len( m_dim_y, part_level, low_len_y, high_len_y );
    
    root.start_x  = 0;
    root.start_y  = 0;
    root.length_x = low_len_x;
    root.length_y = low_len_y;
}


void speck::SPECK2D::m_clean_LIS()
{
    std::vector<SPECKSet2D> tmp;

    for( long i = 0; i < m_LIS_garbage_cnt.size(); i++ )
    {
        // Only consolidate memory if the garbage amount is big enough, 
        // in both absolute and relative senses.
        if( m_LIS_garbage_cnt[i] > m_vec_init_capacity && 
            m_LIS_garbage_cnt[i] > m_LIS[i].size() / 2  )
        {
            tmp.clear();
            tmp.reserve( m_vec_init_capacity );
            for( const auto& s : m_LIS[i] )
                if( !s.garbage )
                    tmp.push_back( s );
            std::swap( m_LIS[i], tmp );
            m_LIS_garbage_cnt[i] = 0;
        }
    }
}


bool speck::SPECK2D::m_ready_to_encode() const
{
    if( m_coeff_buf == nullptr )
        return false;
    if( m_dim_x <= 0 || m_dim_y <= 0 )
        return false;
    if( m_budget == 0 )
        return false;

    return true;
}


bool speck::SPECK2D::m_ready_to_decode() const
{
    if( m_bit_buffer.empty() )
        return false;
    if( m_dim_x <= 0 || m_dim_y <= 0 )
        return false;
    if( m_max_coefficient_bits == 0 )
        return false;
    if( m_budget == 0 )
        return false;

    return true;
}


#ifdef PRINT
void speck::SPECK2D::m_print_set( const char* str, const SPECKSet2D& set ) const
{
    printf( "%s: (%d, %d, %d, %d)\n", str, set.start_x, set.start_y, 
                                       set.length_x, set.length_y );
}
#endif


//
// Class SPECKSet2D
//
bool speck::SPECKSet2D::is_pixel() const
{
    return ( length_x == 1 && length_y == 1 );
}

speck::SPECKSetType speck::SPECKSet2D::type() const
{
    return m_type;
}

// Constructor
speck::SPECKSet2D::SPECKSet2D( SPECKSetType t )
                 : m_type( t )
{ }
