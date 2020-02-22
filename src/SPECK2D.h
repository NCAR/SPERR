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
#define FOUR_BYTE_SPECK_SET

class SPECKSet2D
{
public:

#ifdef FOUR_BYTE_SPECK_SET
    using INT = uint32_t;   // unsigned int
#else
    using INT = int64_t;    // long
#endif
    
    // Member data
    INT  start_x      = 0;
    INT  start_y      = 0;
    INT  length_x     = 0;   
    INT  length_y     = 0;
    INT  part_level   = 0;  // which partition level is this set at (starting from zero).
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
    SPECKSetType type()     const;
};


//
// Main SPECK2D class
//
class SPECK2D
{
public:
    void assign_coeffs( double* );  // Takes ownership of a chunck of memory.
    template <typename T>
    void copy_coeffs( const T* );   // Make a copy of the incoming data.
    void assign_mean_dims( double, long, long ); 
                                    // Accepts data mean and plane  dimensions.
    void assign_bit_budget( long ); // How many bits does speck process? 

    int speck2d();

private:
    //
    // Private methods
    //
    // Return 0 for normal execution, 1 for bit budget met.
    int  m_sorting_pass( );
    int  m_process_S( long idx1, long idx2 );   // Use indices to locate which set to process from m_LIS,
    int  m_code_S(    long idx1, long idx2 );   // because the choice to use vectors to represent lists.
    void m_partition_S( const SPECKSet2D& set, std::array<SPECKSet2D, 4>& subsets ) const;
                          // Partition set into 4 smaller sets, and put them in list.
                          // Note: list will be resized to 4 and contains the 4 subsets.
    int  m_process_I();
    int  m_code_I();
    void m_partition_I( std::array<SPECKSet2D, 3>& subsets );
    int  m_output_set_significance( SPECKSet2D& set );
    int  m_output_pixel_sign( const SPECKSet2D& pixel );
    long m_num_of_partitions() const; 
                          // How many partitions available to perform given the 2D dimensions?
    void m_calc_set_size( SPECKSet2D& set, long subband ) const;
                          // What's the set size and offsets?
                          // subband = (0, 1, 2, 3)
    void m_clean_LIS();   // Clean garbage sets from m_LIS if garbage exists.


    //
    // Private data members
    //
    using buffer_type = std::unique_ptr<double[]>;
    buffer_type m_coeff_buf = nullptr;      // All coefficients are kept here
    double      m_data_mean = 0.0;          // Mean subtracted before DWT
    double      m_threshold = 0.0;          // Threshold that's used for an iteration
    uint64_t    m_budget    = 0;            // What's the budget for num of bits?
    uint64_t    m_bit_cnt   = 0;            // How many bits are already output/input?
    long m_dim_x = 0, m_dim_y = 0;          // 2D plane dims

    std::vector<bool> m_significance_map;
    std::vector<bool> m_sign_array;

    std::vector< SPECKSet2D >               m_LSP;
    std::vector< std::vector<SPECKSet2D> >  m_LIS;
    std::vector< long >                     m_LIS_garbage_cnt;
    SPECKSet2D                              m_I  = SPECKSet2D( SPECKSetType::TypeI );
};

};

#endif
