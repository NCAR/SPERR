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
    INT  part_level   = 0;  // which partition level is this set at (starting from zero)?
    Significance sig  = Significance::Insignificant;
    const SPECKSetType type;

    //
    // Member functions
    //
    // Constructor
    SPECKSet2D( SPECKSetType t );
    bool is_pixel() const;
};


//
// Main SPECK2D class
//
class SPECK2D
{
public:
    void assign_coeffs( double* );  // Takes ownership of a chunck of memory
    void assign_mean_dims( double, long, long ); 
                                    // Accepts data mean and volume dimensions.
    int speck2d();

private:
    //
    // Private methods
    //
    void m_sorting_pass( const double threshold );
    void m_process_S( SPECKSet2D& set );
    void m_output_set_significance( SPECKSet2D& set ) const;
    long m_num_of_part_levels() const; 
                          // How many partition levels are there given the 2D dimensions?
    void m_calc_set_size( SPECKSet2D& set, long subband ) const;
                          // What's the set size and offsets?
                          // subband = (0, 1, 2, 3)

    double m_make_positive(); // 1) fill m_sign_array based on m_data_buf signs, and 
                              // 2) make m_data_buf containing all positive values.
                              // Returns the maximum magnitude of all encountered values.
    void m_update_significance_map( const double threshold );
                          // Update the significance map according to threshold.


    //
    // Private data members
    //
    using buffer_type = std::unique_ptr<double[]>;
    buffer_type m_coeff_buf = nullptr;              // All coefficients are kept here
    double      m_data_mean = 0.0;                  // Mean subtracted before DWT
    long m_dim_x = 0, m_dim_y = 0;                  // 2D plane dims

    std::vector<bool> m_significance_map;
    std::vector<bool> m_sign_array;

    std::vector< SPECKSet2D >               m_LSP;
    std::vector< std::vector<SPECKSet2D> >  m_LIS;
    SPECKSet2D                              m_I = SPECKSet2D( SPECKSetType::TypeI );
};

};

#endif
