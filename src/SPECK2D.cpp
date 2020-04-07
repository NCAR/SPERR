#include "SPECK2D.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <array>
#include <algorithm>


// Constructor
speck::SPECK2D::SPECK2D()
{
    m_I.type = SetType::TypeI;
}

void speck::SPECK2D::assign_dims( size_t dx, size_t dy )
{
    // Sanity Check
    assert( m_coeff_len == 0 || m_coeff_len == dx * dy );
    m_dim_x = dx;
    m_dim_y = dy;
    m_coeff_len = dx * dy;
}


void speck::SPECK2D::assign_max_coeff_bits( uint16_t bits )
{
    m_max_coefficient_bits = bits;
}


uint16_t speck::SPECK2D::get_max_coeff_bits() const
{
    return m_max_coefficient_bits;
}


void speck::SPECK2D::assign_bit_budget( size_t budget )
{
    size_t mod = budget % 8;
    if( mod == 0 )
        m_budget = budget;
    else    // we can fill up that last byte!
        m_budget = budget + 8 - mod;
}
    

int speck::SPECK2D::encode()
{
    assert( m_ready_to_encode() );
    m_encode_mode = true;

    m_initialize_sets_lists();

    // Get ready for the quantization loop!
    m_bit_buffer.clear();
    m_bit_buffer.reserve( m_budget + m_vec_init_capacity );
    auto max_coeff = speck::make_coeff_positive( m_coeff_buf, m_dim_x * m_dim_y,
                                                 m_sign_array );
    m_max_coefficient_bits = uint16_t( std::log2(max_coeff) );
    /* When max_coeff is very close to zero, m_max_coefficient_bits could be zero.
       I don't know how to deal with that situation yet...                      */
    assert( m_max_coefficient_bits > 0 );   
    m_threshold = std::pow( 2.0f, float(m_max_coefficient_bits) );
    int rtn = 0;
    for( size_t bitplane = 0; bitplane < 128; bitplane++ )
    {
        if( (rtn = m_sorting_pass()) )
            break;
        if( (rtn = m_refinement_pass()) )
            break;

        m_threshold *= 0.5f;

        m_clean_LIS();
    }

    return rtn;
}


int speck::SPECK2D::decode()
{
    assert( m_ready_to_decode() );
    m_encode_mode = false;

#ifdef PRINT
    std::cout << "<-- start decoding -->" << std::endl;
#endif

    // initialize coefficients to be zero, and signs to be all positive
#ifdef SPECK_USE_DOUBLE
    m_coeff_buf = std::make_unique<double[]>( m_coeff_len );
#else
    m_coeff_buf = std::make_unique<float[]>( m_coeff_len );
#endif
    for( size_t i = 0; i < m_coeff_len; i++ )
        m_coeff_buf[i] = 0.0f;
    m_sign_array.assign( m_coeff_len, true );

    m_initialize_sets_lists();
    
    m_bit_idx = 0;
    m_threshold = std::pow( 2.0f, float(m_max_coefficient_bits) );
    int rtn_val = 0;
    for( size_t bitplane = 0; bitplane < 128; bitplane++ )
    {
        if( (rtn_val = m_sorting_pass()) )
            break;
        if( (rtn_val = m_refinement_pass()) )
            break;

        m_threshold *= 0.5f;

        m_clean_LIS();
    }

    // Restore coefficient signs
    for( size_t i = 0; i < m_sign_array.size(); i++ )
    {
        if( !m_sign_array[i] )
            m_coeff_buf[i] *= -1.0f;
    }

    return rtn_val;
}


void speck::SPECK2D::m_initialize_sets_lists()
{
    const auto num_of_parts  = m_num_of_partitions();
    const auto num_of_xforms = speck::calc_num_of_xforms( std::min( m_dim_x, m_dim_y) );

    // prepare m_LIS
    m_LIS.clear();
    m_LIS.resize( num_of_parts + 1 );
    for( auto& v : m_LIS )  // Avoid frequent memory allocations.
        v.reserve( m_vec_init_capacity );
    m_LIS_garbage_cnt.assign( num_of_parts + 1, 0 );

    // prepare the root, S
    SPECKSet2D S;
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
        m_significance_map.assign( m_coeff_len, false );
        for( size_t i = 0; i < m_coeff_len; i++ )
        {
            if( m_coeff_buf[i] >= m_threshold )
                m_significance_map[i] = true;
        }
    }

    int rtn = 0;
    for( size_t tmp = 0; tmp < m_LIS.size(); tmp++ )
    {
        // From the end to the front of m_LIS, smaller sets first.
        size_t idx1 = m_LIS.size() - 1 - tmp;
        for( size_t idx2  = 0; idx2 < m_LIS[idx1].size(); idx2++ )
        {
            auto& s = m_LIS[idx1][idx2];
            if( s.type != SetType::Garbage )
            {
                if( (rtn = m_process_S( idx1, idx2, true )) )
                    return rtn;
            }
        }
    }

    if( (rtn = m_process_I( true )) )
        return rtn;

    return 0;
}


int speck::SPECK2D::m_refinement_pass( )
{
#ifdef PRINT
    printf("--> refinement pass, threshold = %f\n", m_threshold );
#endif

    int rtn = 0;
    for( auto& p : m_LSP )
    {
        if( p.signif == Significance::NewlySig )
            p.signif  = Significance::Sig;
        else
        {
            if( m_encode_mode )
            {
                if( (rtn = m_output_refinement( p )) )
                    return rtn;
            }
            else
            {
                if( (rtn = m_input_refinement( p )) )
                    return rtn;
            }
        }
    }

    return 0;
}


int speck::SPECK2D::m_process_S( size_t idx1, size_t idx2, bool need_decide_signif )
{
    auto& set = m_LIS[idx1][idx2];

#ifdef PRINT
    m_print_set( "process_S", set );
#endif

    int rtn = 0;
    if( need_decide_signif )
    {
        if( m_encode_mode )
        {
            m_decide_set_significance( set );
            if( (rtn = m_output_set_significance( set )) )
                return rtn;
        }
        else
        {
            // In decode mode, this function reads a bit.
            if( (rtn = m_decide_set_significance( set )) )
                return rtn;
#ifdef PRINT
    auto bit = ( set.signif == Significance::Sig );
    std::string str = bit ? "s1" : "s0";
    std::cout << str << std::endl;
#endif
        }
    }
    else
        set.signif = Significance::Sig;

    if( set.signif == Significance::Sig )
    {
        if( set.is_pixel() )
        {
            set.signif = Significance::NewlySig;
            if( m_encode_mode )
            {
                if( (rtn = m_output_pixel_sign( set )) )
                    return rtn;
            }
            else
            {
                if( (rtn = m_input_pixel_sign( set )) )
                    return rtn;
            }
            m_LSP.push_back( set );         // A copy is saved to m_LSP.
            set.type = SetType::Garbage ;   // This particular object will be discarded.
            m_LIS_garbage_cnt[ set.part_level ]++;
        }
        else
        {
            if( (rtn = m_code_S( idx1, idx2 )) )
                return rtn;
            set.type = SetType::Garbage;   // This particular object will be discarded.
            m_LIS_garbage_cnt[ set.part_level ]++;
        }
    }

    return 0;
}


int speck::SPECK2D::m_code_S( size_t idx1, size_t idx2 )
{
    const auto& set = m_LIS[idx1][idx2];
    
#ifdef PRINT
    m_print_set( "code_S", set );
#endif

    std::array< SPECKSet2D, 4 > subsets;
    m_partition_S( set, subsets );

    // We count how many subsets are significant, and if the first 3 subsets ain't,
    // then the 4th one must be significant.
    int already_sig = 0, rtn = 0;
    for( size_t i = 0; i < 3; i++ )
    {
        auto& s = subsets[i];
        if( !s.is_empty() )
        {
            m_LIS[ s.part_level ].push_back( s );
            size_t newidx1 = s.part_level;
            size_t newidx2 = m_LIS[ newidx1 ].size() - 1;
            if( (rtn = m_process_S( newidx1, newidx2, true )) )
                return rtn;

            if( m_LIS[ newidx1 ][ newidx2 ].signif == Significance::Sig ||
                m_LIS[ newidx1 ][ newidx2 ].signif == Significance::NewlySig )
                already_sig++;
        }
    }

    auto& s4 = subsets[3];
    if( !s4.is_empty() )
    {
        bool need_decide_sig = already_sig == 0 ? false : true;
        m_LIS[ s4.part_level ].push_back( s4 );
        if( (rtn = m_process_S( s4.part_level, m_LIS[s4.part_level].size() - 1, need_decide_sig )) )
            return rtn;
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


int speck::SPECK2D::m_process_I( bool need_decide_sig )
{
    if( m_I.part_level == 0 )   // m_I is empty at this point
        return 0;
    
#ifdef PRINT
    m_print_set( "process_I", m_I );
#endif

    int rtn = 0;
    if( m_encode_mode )
    {
        if( need_decide_sig )
            m_decide_set_significance( m_I );
        else
            m_I.signif = Significance::Sig;
        if( (rtn = m_output_set_significance( m_I )) )
            return rtn;
    }
    else
    {
        if( (rtn = m_decide_set_significance( m_I )) )
            return rtn;
#ifdef PRINT
    auto bit = ( m_I.signif == Significance::Sig );
    std::string str = bit ? "s1" : "s0";
    std::cout << str << std::endl;
#endif
    }

    if( m_I.signif == Significance::Sig )
    {
        if( (rtn = m_code_I()) )
            return rtn;
    }

    return 0;
}


int speck::SPECK2D::m_code_I()
{
    std::array< SPECKSet2D, 3 > subsets;
    m_partition_I( subsets );

    // We count how many subsets are significant, and if the 3 subsets resulted from
    // m_partition_I() are all insignificant, then it must be the remaining m_I to be
    // significant.
    int already_sig = 0, rtn = 0;
    for( size_t i = 0; i < 3; i++ )
    {
        auto& s = subsets[i];
        if( !s.is_empty() )
        {
            m_LIS[ s.part_level ].push_back( s );
            size_t newidx1 = s.part_level;
            size_t newidx2 = m_LIS[ newidx1 ].size() - 1;
            if( (rtn = m_process_S( newidx1, newidx2, true )) )
                return rtn;

            if( m_LIS[ newidx1 ][ newidx2 ].signif == Significance::Sig ||
                m_LIS[ newidx1 ][ newidx2 ].signif == Significance::NewlySig )
                already_sig++;
        }
    }

    bool need_decide_sig = already_sig == 0 ? false : true;
    if( (rtn = m_process_I( need_decide_sig )) )
        return rtn;

    return 0;
}


void speck::SPECK2D::m_partition_I( std::array<SPECKSet2D, 3>& subsets )
{
    std::array<size_t, 2> len_x, len_y;
    speck::calc_approx_detail_len( m_dim_x, m_I.part_level, len_x );
    speck::calc_approx_detail_len( m_dim_y, m_I.part_level, len_y );
    const auto approx_len_x = len_x[0];
    const auto detail_len_x = len_x[1];
    const auto approx_len_y = len_y[0];
    const auto detail_len_y = len_y[1];

    // specify the subsets following the same order in QccPack
    auto& BR      = subsets[0];         // Bottom right
    BR.part_level = m_I.part_level;
    BR.start_x    = approx_len_x;
    BR.start_y    = approx_len_y;
    BR.length_x   = detail_len_x;
    BR.length_y   = detail_len_y;

    auto& TR      = subsets[1];         // Top right
    TR.part_level = m_I.part_level;
    TR.start_x    = approx_len_x;
    TR.start_y    = 0;
    TR.length_x   = detail_len_x;
    TR.length_y   = approx_len_y;

    auto& BL      = subsets[2];         // Bottom left
    BL.part_level = m_I.part_level;
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
    // Note: the only case this method returns 1 is when decoding and we have 
    //       already decoded enough number of bits.
    //       All other cases return 0, meaning successfully decided the 
    //       significance of this set.  

    // If decoding, simply read a bit from the bitstream, no matter TypeS or TypeI.
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
    if( set.type == SetType::TypeS )
    {
        for( auto y = set.start_y; y < (set.start_y + set.length_y); y++ )
            for( auto x = set.start_x; x < (set.start_x + set.length_x); x++ )
            {
                auto idx = y * m_dim_x + x;
                if( m_significance_map[ idx ] )
                {
                    set.signif = Significance::Sig;
                    return 0;
                }
            }
    }
    // For TypeI sets, we need to test two rectangles!
    else if( set.type == SetType::TypeI )   
    {
        // First rectangle: directly to the right of the missing top-left corner
        for( size_t y = 0; y < set.start_y; y++ )
            for( auto x = set.start_x; x < set.length_x; x++ )
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
        for( auto i = set.start_y * set.length_x; i < set.length_x * set.length_y; i++ )
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
    const auto idx = pixel.start_y * m_dim_x + pixel.start_x;

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

    const auto idx = pixel.start_y * m_dim_x + pixel.start_x ;
    m_sign_array[ idx ] = m_bit_buffer[ m_bit_idx++ ];

    // Progressive quantization!
    m_coeff_buf[ idx ] = 1.5f * m_threshold;

#ifdef PRINT
    auto bit = m_sign_array[ idx ];
    std::string str = bit ? "p1" : "p0";
    std::cout << str << std::endl;
#endif

    return 0;
}


int speck::SPECK2D::m_output_refinement( const SPECKSet2D& pixel )
{
    const auto idx = pixel.start_y * m_dim_x + pixel.start_x;

    if( m_coeff_buf[idx] >= m_threshold ) 
    {
        m_bit_buffer.push_back( true );
#ifdef PRINT
        std::cout << "r1" << std::endl;
#endif
        m_coeff_buf[idx] -= m_threshold;
    }
    else
    {
        m_bit_buffer.push_back( false );
#ifdef PRINT
        std::cout << "r0" << std::endl;
#endif
    }

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

    const auto bit = m_bit_buffer[ m_bit_idx++ ];
    const auto idx = pixel.start_y * m_dim_x + pixel.start_x;
    m_coeff_buf[ idx ] += bit ? m_threshold * 0.5f : m_threshold * -0.5f;

#ifdef PRINT
    if( bit )
        std::cout << "r1" << std::endl;
    else
        std::cout << "r0" << std::endl;
#endif

    return 0;
}

    
// Calculate the number of partitions able to be performed
size_t speck::SPECK2D::m_num_of_partitions() const
{
    size_t num_of_parts = 0;
    size_t dim_x = m_dim_x, dim_y = m_dim_y;
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
    // approximation and detail lengths are placed as the 1st and 2nd element
    std::array<size_t, 2> len_x, len_y;
    speck::calc_approx_detail_len( m_dim_x, root.part_level, len_x );
    speck::calc_approx_detail_len( m_dim_y, root.part_level, len_y );
    
    root.start_x  = 0;
    root.start_y  = 0;
    root.length_x = len_x[0];
    root.length_y = len_y[0];
}


void speck::SPECK2D::m_clean_LIS()
{
    std::vector<SPECKSet2D> tmp;

    for( size_t i = 0; i < m_LIS_garbage_cnt.size(); i++ )
    {
        // Only consolidate memory if the garbage amount is big enough, 
        // in both absolute and relative senses.
        if( m_LIS_garbage_cnt[i] >  m_vec_init_capacity && 
            m_LIS_garbage_cnt[i] >= m_LIS[i].size() / 2  )
        {
            tmp.clear();
            tmp.reserve( m_vec_init_capacity );
            for( const auto& s : m_LIS[i] )
                if( s.type != SetType::Garbage )
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

bool speck::SPECKSet2D::is_empty() const
{
    return ( length_x == 0 || length_y == 0 );
}
