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

class SPECKSet2D
{
public:
    // Member data
    long start_x      = 0;
    long start_y      = 0;
    long length_x     = 0;   
    long length_y     = 0;
    int  part_level   = 0;  // which partition level is this set at?
                            // Since there'll be a large number of these sets generated,
                            // let's use int here (to reduce 8 bytes for this structure).
    bool significance = false;
    SPECKSetType type;

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

    void m_calc_set_size_2d( SPECKSet2D& set, long subband ) const;
                          // What's the set size and offsets?
                          // subband = (0, 1, 2, 3)
private:
    //
    // Private methods
    //
    double m_make_positive( std::vector<bool>& sign_array ) const; 
                          // 1) fill m_sign_array based on m_data_buf signs, and 
                          // 2) make m_data_buf containing all positive values.
                          // Returns the maximum magnitude of all encountered values.
    long m_num_of_part_levels_2d() const; 
                          // How many partition levels are there given the 2D dimensions?


    //
    // Private data members
    //
    using buffer_type = std::unique_ptr<double[]>;
    buffer_type m_coeff_buf = nullptr;              // All coefficients are kept here
    double      m_data_mean = 0.0;                  // Mean subtracted before DWT
    long m_dim_x = 0, m_dim_y = 0, m_dim_z = 0;     // Volume dims
};

};

#endif
