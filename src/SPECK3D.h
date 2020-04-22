#ifndef SPECK3D_H
#define SPECK3D_H

#include "SPECK_Storage.h"

// #define PRINT

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
    uint32_t  start_x      = 0;
    uint32_t  start_y      = 0;
    uint32_t  start_z      = 0;
    uint32_t  length_x     = 0;
    uint32_t  length_y     = 0;
    uint32_t  length_z     = 0;
    // which partition level is this set at (starting from zero, in all 3 directions).
    uint16_t part_level_x  = 0;  
    uint16_t part_level_y  = 0;  
    uint16_t part_level_z  = 0;  
    Significance signif = Significance::Insig;
    SetType type = SetType::TypeS;  // This field is only used to indicate garbage status

public:
    //
    // Member functions
    //
    bool   is_pixel() const;
    bool   is_empty() const;
    size_t total_partitions() const;
#ifdef PRINT
    void print() const;
#endif
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
    int encode();
    int decode();
    virtual int write_to_disk( const std::string& filename ) const override;

private:
    //
    // Note: for methods returning an integer, 0 means normal execution, and
    // 1 means bit budget met. 
    //
    bool m_ready_to_encode() const;
    bool m_ready_to_decode() const;
    // How many partition operation could we perform in each direction?
    void m_num_of_partitions( std::array<size_t, 3>& ) const; 
    void m_clean_LIS();   // Clean garbage sets from m_LIS if too much garbage exists.
    void m_initialize_sets_lists();
    int  m_sorting_pass( );
    int  m_refinement_pass( );

    // For the following 2 methods, indices are used to locate which set to process from m_LIS,
    int  m_process_S( size_t idx1, size_t idx2 );
    int  m_code_S   ( size_t idx1, size_t idx2 );

    // It decides the significance of "set" by looking up the significance map, and store the 
    //   information of its 8 descendants in "sigs." 
    //void m_lookup_significance_map( SPECKSet3D& set, std::array<Significance, 8>& sigs );
    int  m_input_set_significance(        SPECKSet3D& set );
    int  m_output_set_significance( const SPECKSet3D& set );
    int  m_input_pixel_sign(        const SPECKSet3D& pixel );
    int  m_output_pixel_sign(       const SPECKSet3D& pixel );
    int  m_input_refinement(        const SPECKSet3D& pixel );
    int  m_output_refinement(       const SPECKSet3D& pixel );

    void m_partition_S_XYZ(const SPECKSet3D& set, std::array<SPECKSet3D, 8>& subsets ) const;
    void m_partition_S_XY( const SPECKSet3D& set, std::array<SPECKSet3D, 4>& subsets ) const;
    void m_partition_S_Z(  const SPECKSet3D& set, std::array<SPECKSet3D, 2>& subsets ) const;

    //
    // Private data members
    //
#ifdef SPECK_USE_DOUBLE
    double      m_threshold = 0.0;          // Threshold that's used for an iteration
#else
    float       m_threshold = 0.0f;
#endif
    size_t      m_budget      = 0;          // What's the budget for num of bits?
    size_t      m_bit_idx     = 0;          // Used for decode. Which bit we're at?
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
};

};

#endif
