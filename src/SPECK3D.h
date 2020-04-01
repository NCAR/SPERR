#ifndef SPECK3D_H
#define SPECK3D_H

#include "SPECK_Storage.h"

namespace speck
{

//
// Auxiliary class to hold a SPECK Set
//
class SPECKSet3D
{
public:
    //
    // Member data
    //
    UINT  start_x       = 0;
    UINT  start_y       = 0;
    UINT  start_z       = 0;
    UINT  length_x      = 0;    // For typeI set, this value equals to m_dim_x.
    UINT  length_y      = 0;    // For typeI set, this value equals to m_dim_y.
    UINT  length_z      = 0;    // For typeI set, this value equals to m_dim_z.
    // which partition level is this set at (starting from zero, in both directions).
    uint16_t part_level_xy = 0;  
    uint16_t part_level_z  = 0;  
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
// Main SPECK3D class
//
class SPECK3D : public SPECK_Storage
{
public:
    // trivial input
    void assign_dims( size_t, size_t, size_t ); // Accepts volume dimension
    void assign_max_coeff_bits( uint16_t );     // (Useful for reconstruction)
    void assign_bit_budget( size_t );           // How many bits does speck process? 

    // trivial output
    uint16_t get_max_coeff_bits()   const;

    // core operations
    //int encode();
    //int decode();

private:
    //
    // Note: for methods returning an integer, 0 means normal execution, and
    // 1 means bit budget met. 
    //
    void m_clean_LIS();   // Clean garbage sets from m_LIS if too much garbage exists.
    // In encoding mode, it examines the significance of "set," and store the information
    //  of its 8 descendants in "sigs."
    //  It always returns 0 in this mode.
    // In decoding mode, it simply reads a bit from the bit buffer.
    //  In this mode, it returns 1 to indicate bit budget met, 0 otherwise. 
    int  m_decide_set_S_significance( SPECKSet3D& set, std::array<Significance, 8>& sigs );
    int  m_decide_set_I_significance( SPECKSet3D& set, std::array<Significance, 7>& sigs );
    int  m_output_set_significance( const SPECKSet3D& set );

#if 0
    int  m_sorting_pass( );
    int  m_refinement_pass( );
    // For the following 2 methods, indices are used to locate which set to process from m_LIS,
    // because of the choice to use vectors to represent lists, only indices are invariant. 
    int  m_process_S( size_t idx1, size_t idx2, bool ); // need to decide if it's signif?
    int  m_code_S   ( size_t idx1, size_t idx2 );
    int  m_process_I( );
    int  m_code_I   ( );
    void m_initialize_sets_lists();
    void m_partition_S( const SPECKSet2D& set, std::array<SPECKSet2D, 4>& subsets ) const;
    void m_partition_I( std::array<SPECKSet2D, 3>& subsets );
    int  m_input_pixel_sign(  const SPECKSet2D& pixel );
    int  m_output_pixel_sign( const SPECKSet2D& pixel );
    int  m_input_refinement(  const SPECKSet2D& pixel );
    int  m_output_refinement( const SPECKSet2D& pixel );

    void m_calc_root_size( SPECKSet2D& root ) const;
    // How many partitions available to perform given the 2D dimensions?
    size_t m_num_of_partitions() const; 
    bool m_ready_to_encode() const;
    bool m_ready_to_decode() const;
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
    size_t      m_dim_x       = 0;          // 3D volume dims
    size_t      m_dim_y       = 0;
    size_t      m_dim_z       = 0;
    bool        m_encode_mode = true;       // Encode (true) or Decode (false) mode?
    const size_t m_vec_init_capacity = 16;  // Vectors are initialized to have this capacity.

    std::vector<bool> m_significance_map;
    std::vector<bool> m_sign_array;

    std::vector< SPECKSet3D >               m_LSP;
    std::vector< std::vector<SPECKSet3D> >  m_LIS;
    std::vector< size_t >                   m_LIS_garbage_cnt;
    SPECKSet3D                              m_I;
};

};

#endif
