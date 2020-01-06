#ifndef SPECK_H
#define SPECK_H

#include <memory>
#include <vector>

namespace speck
{

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
    double m_make_positive( std::vector<bool>& sign_array ) const; 
                          // 1) fill m_sign_array based on m_data_buf signs, and 
                          // 2) make m_data_buf containing all positive values.
                          // Returns the maximum magnitude of all encountered values.
    long m_num_of_levels_xy() const;  // How many levels of DWT on the XY plane?
    long m_num_of_levels_z()  const;  // How many levels of DWT on the Z direction?
    long m_calc_approx_len( long orig_len, long lev ) const;
                          // Determine the low frequency signal length at level lev
                          // (0 <= lev < num_level_xy/z).

    //
    // Private data members
    //
    using buffer_type = std::unique_ptr<double[]>;
    buffer_type m_coeff_buf = nullptr;              // All coefficients are kept here
    double      m_data_mean = 0.0;                  // Mean subtracted before DWT
    long m_dim_x = 0, m_dim_y = 0, m_dim_z = 0;     // Volume dims
};


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
    long origin_row;
    long origin_col;
    long num_rows;   
    long num_cols;
    long level        = 0;
    bool significance = false;
    SPECKSetType type;

    //
    // Member functions
    //
    // Constructor
    SPECKSet2D( SPECKSetType t, long o_r = 0, long o_c = 0, long num_r = 0, long num_c  = 0 );
    bool is_single_pixel() const;
};

};

#endif
