#ifndef SPECK2D_H
#define SPECK2D_H

#include "speck_helper.h"

#include <memory>
#include <vector>

namespace speck
{
//
// Auxiliary class to hold a SPECK Set
//   Comment out the following macro will double the size of SPECKSet2D
//   from 24 bytes to 48 bytes.
//

class SPECKSet2D
{
public:

#ifdef EIGHT_BYTE_SPECK_SET
    using UINT = uint64_t;   // unsigned long
#else
    using UINT = uint32_t;   // unsigned int
#endif
    
    // Member data
    UINT  start_x       = 0;
    UINT  start_y       = 0;
    UINT  length_x      = 0;   
    UINT  length_y      = 0;
    uint16_t part_level = 0;  // which partition level is this set at (starting from zero).
    Significance signif = Significance::Insig;
    bool garbage        = false;

private:
    SPECKSetType m_type = SPECKSetType::TypeS;

public:
    //
    // Member functions
    //
    // Constructor
    SPECKSet2D() = default;
    SPECKSet2D( SPECKSetType t );

    bool         is_pixel() const;
    bool         is_empty() const;
    SPECKSetType type()     const;
};


//
// Main SPECK2D class
//
class SPECK2D
{
public:
    // memory management: input
    void take_coeffs( std::unique_ptr<double[]> );  // Take ownership of a chunck of memory.
    template <typename T>
    void copy_coeffs( const T* );                   // Make a copy of the incoming data.
    void take_bitstream( std::vector<bool>& );      // Take ownership of the bitstream.
    void copy_bitstream( const std::vector<bool>& );// Make a copy of the bitstream.

    // memory management: output
    const std::vector<bool>&  get_read_only_bitstream() const;
    std::vector<bool>&        release_bitstream();          // The bitstream will be up to changes.
    const double*             get_read_only_coeffs() const; // Others can read the data
    std::unique_ptr<double[]> release_coeffs();             // Others take ownership of the data

    // trivial properties
    void     assign_mean( double );             // Accepts data mean.
    void     assign_dims( size_t, size_t );     // Accepts plane dimension
    void     assign_max_coeff_bits( uint16_t ); // (Useful for reconstruction)
    void     assign_bit_budget( size_t );       // How many bits does speck process? 
    uint16_t get_max_coeff_bits()   const;

    // debug use output
    size_t get_bit_idx() const;
    size_t get_bit_buffer_size() const;

    // core operations
    int encode();
    int decode();

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
    int  m_process_I( );
    int  m_code_I   ( );
    void m_initialize_sets_lists();
    void m_partition_S( const SPECKSet2D& set, std::array<SPECKSet2D, 4>& subsets ) const;
    void m_partition_I( std::array<SPECKSet2D, 3>& subsets );
    int  m_decide_set_significance( SPECKSet2D& set );  // both encoding and decoding
    int  m_output_set_significance( const SPECKSet2D& set );
    int  m_output_pixel_sign( const SPECKSet2D& pixel );
    int  m_input_pixel_sign(  const SPECKSet2D& pixel );
    int  m_output_refinement( const SPECKSet2D& pixel );
    int  m_input_refinement(  const SPECKSet2D& pixel );

    void m_calc_root_size( SPECKSet2D& root ) const;
    // How many partitions available to perform given the 2D dimensions?
    size_t m_num_of_partitions() const; 
    void m_clean_LIS();   // Clean garbage sets from m_LIS if too much garbage exists.
    bool m_ready_to_encode() const;
    bool m_ready_to_decode() const;

    // 1) fill m_sign_array based on data_buf signs, and 
    // 2) make m_coeff_buf containing all positive values.
    // Returns the maximum magnitude of all encountered values.
    double m_make_coeff_positive( ); 

#ifdef PRINT
    void m_print_set( const char*, const SPECKSet2D& set ) const;
#endif


    //
    // Private data members
    //
    using buffer_type = std::unique_ptr<double[]>;
    buffer_type m_coeff_buf = nullptr;      // All coefficients are kept here
    double      m_data_mean = 0.0;          // Mean subtracted before DWT
    double      m_threshold = 0.0;          // Threshold that's used for an iteration
    size_t      m_budget    = 0;            // What's the budget for num of bits?
    size_t      m_bit_idx   = 0;            // Used for decode. Which bit we're at?
    size_t      m_max_coefficient_bits = 0; // = log2(max_coefficient)
    size_t      m_dim_x       = 0;          // 2D plane dims
    size_t      m_dim_y       = 0;
    bool        m_encode_mode = true;       // Encode (true) or Decode (false) mode?
    const size_t m_vec_init_capacity = 8;   // Vectors are initialized to have this capacity.

    std::vector<bool> m_significance_map;
    std::vector<bool> m_sign_array;
    std::vector<bool> m_bit_buffer;

    std::vector< SPECKSet2D >               m_LSP;
    std::vector< std::vector<SPECKSet2D> >  m_LIS;
    std::vector< size_t >                   m_LIS_garbage_cnt;
    SPECKSet2D                              m_I  = SPECKSet2D( SPECKSetType::TypeI );
};

};

#endif
