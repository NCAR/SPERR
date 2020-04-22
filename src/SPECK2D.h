#ifndef SPECK2D_H
#define SPECK2D_H

#include "SPECK_Storage.h"

namespace speck
{

//
// Auxiliary class to hold a SPECK Set
//
class SPECKSet2D
{
public:
    //
    // Member data
    //
    uint32_t  start_x   = 0;
    uint32_t  start_y   = 0;
    uint32_t  length_x  = 0;  // For typeI set, this value equals to m_dim_x
    uint32_t  length_y  = 0;  // For typeI set, this value equals to m_dim_y
    uint16_t part_level = 0;  // which partition level is this set at (starting from zero).
    Significance signif = Significance::Insig;
    SetType type        = SetType::TypeS;

public:
    //
    // Member functions
    //
    bool is_pixel() const;
    bool is_empty() const;
};


//
// Main SPECK2D class
//
class SPECK2D : public SPECK_Storage
{
public:
    // Constructor
    SPECK2D();

    // trivial input
    void  assign_dims( size_t, size_t );       // Accepts plane dimension
    void  assign_max_coeff_bits( uint16_t );   // (Useful for reconstruction)
    void  assign_bit_budget( size_t );         // How many bits does speck process? 

    // trivial output
    //uint16_t get_max_coeff_bits()   const;
    void  get_dims( size_t& , size_t& ) const; // Returns plane dimension

    // core operations
    int encode();
    int decode();
    int write_to_disk(  const std::string& filename ) const override;
    int read_from_disk( const std::string& filename )       override;

private:
    //
    // Note: for methods returning an integer, 0 means normal execution, and
    // 1 means bit budget met. 
    //
    int  m_sorting_pass( );
    int  m_refinement_pass( );
    // For the following 2 methods, indices are used to locate which set to process from m_LIS,
    // because of the choice to use vectors to represent lists, only indices are invariant. 
    int  m_process_S( size_t idx1, size_t idx2, bool ); // need to decide if it's signif?
    int  m_code_S   ( size_t idx1, size_t idx2 );
    int  m_process_I( bool );   // Need to decide if m_I is significant? 
    int  m_code_I   ( );
    void m_initialize_sets_lists();
    void m_partition_S( const SPECKSet2D& set, std::array<SPECKSet2D, 4>& subsets ) const;
    void m_partition_I( std::array<SPECKSet2D, 3>& subsets );
    int  m_decide_set_significance( SPECKSet2D& set );  // input when decoding
    int  m_output_set_significance( const SPECKSet2D& set );
    int  m_input_pixel_sign(  const SPECKSet2D& pixel );
    int  m_output_pixel_sign( const SPECKSet2D& pixel );
    int  m_input_refinement(  const SPECKSet2D& pixel );
    int  m_output_refinement( const SPECKSet2D& pixel );

    void m_calc_root_size( SPECKSet2D& root ) const;
    // How many partitions available to perform given the 2D dimensions?
    size_t m_num_of_partitions() const; 
    void m_clean_LIS();   // Clean garbage sets from m_LIS if too much garbage exists.
    bool m_ready_to_encode() const;
    bool m_ready_to_decode() const;

#ifdef PRINT
    void m_print_set( const char*, const SPECKSet2D& set ) const;
#endif


    //
    // Private data members
    //
#ifdef SPECK_USE_DOUBLE
    double      m_threshold = 0.0;          // Threshold that's used for an iteration
#else
    float       m_threshold = 0.0f;
#endif
    size_t      m_budget    = 0;            // What's the budget for num of bits?
    size_t      m_bit_idx   = 0;            // Used for decode. Which bit we're at?
    uint16_t    m_max_coefficient_bits = 0; // = log2(max_coefficient)
    size_t      m_dim_x       = 0;          // 2D plane dims
    size_t      m_dim_y       = 0;
    bool        m_encode_mode = true;       // Encode (true) or Decode (false) mode?
    const size_t m_vec_init_capacity = 16;  // Vectors are initialized to have this capacity.

    std::vector<bool> m_significance_map;
    std::vector<bool> m_sign_array;

    std::vector< SPECKSet2D >               m_LSP;
    std::vector< std::vector<SPECKSet2D> >  m_LIS;
    std::vector< size_t >                   m_LIS_garbage_cnt;
    SPECKSet2D                              m_I; 
};

};

#endif
