#ifndef SPECK_H
#define SPECK_H

#include <memory>
#include <vector>

namespace speck
{
//
// Helper classes
//
enum class SPECKSetType : unsigned char
{
    TypeI,
    TypeS
};

enum class Significance : unsigned char
{
    Insignificant,
    Significant,
    Newly_Significant
};


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
    bool is_single_pixel() const;
};


//
// Main SPECK class
//
class SPECK
{
public:
    void assign_coeffs( double* );  // Takes ownership of a chunck of memory
    void assign_mean_dims( double, long, long, long ); 
                                    // Accepts data mean and volume dimensions.
    int speck2d();

private:
    //
    // Private methods
    //
    void m_sorting_pass( const double threshold );
    long m_num_of_part_levels_2d() const; 
                          // How many partition levels are there given the 2D dimensions?
    double m_make_positive( std::vector<bool>& sign_array ) const; 
                          // 1) fill m_sign_array based on m_data_buf signs, and 
                          // 2) make m_data_buf containing all positive values.
                          // Returns the maximum magnitude of all encountered values.
    void m_calc_set_size_2d( SPECKSet2D& set, long subband ) const;
                          // What's the set size and offsets?
                          // subband = (0, 1, 2, 3)
    void m_update_significance_map( const double threshold );
                          // Update the significance map according to threshold.


    //
    // Private data members
    //
    using buffer_type = std::unique_ptr<double[]>;
    buffer_type m_coeff_buf = nullptr;              // All coefficients are kept here
    double      m_data_mean = 0.0;                  // Mean subtracted before DWT
    long m_dim_x = 0, m_dim_y = 0, m_dim_z = 0;     // Volume dims

    std::vector<bool> m_significance_map;

    // Lists and sets specifically for 2D encoding
    std::vector< SPECKSet2D >               m_LSP_2d;
    std::vector< std::vector<SPECKSet2D> >  m_LIS_2d;
    SPECKSet2D   m_I_2d = SPECKSet2D( SPECKSetType::TypeI );
};

};

#endif
