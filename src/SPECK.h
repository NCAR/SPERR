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
    double m_make_positive(); // 1) fill m_sign_array based on m_data_buf signs, and 
                              // 2) make m_data_buf containing all positive values.
                              // Returns the maximum magnitude of all encountered values.

    //
    // Private data members
    //
    using buffer_type = std::unique_ptr<double[]>;
    buffer_type m_coeff_buf = nullptr;              // All coefficients are kept here
    double      m_data_mean = 0.0;                  // Mean subtracted before DWT
    long m_max_coefficient_bits = 0;                // How many bits needed
    long m_dim_x = 0, m_dim_y = 0, m_dim_z = 0;     // Volume dims
    std::vector<bool> m_sign_array;                 // Note: vector<bool> is a lot faster 
    std::vector<bool> m_significance_map;           //   and memory-efficient
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
    long level      = 0;
    long origin_row = 0;
    long origin_col = 0;
    long num_rows   = 0;
    long num_cols   = 0;
    bool significance = false;
    SPECKSetType type;

    // Member functions
    bool is_single_pixel() const;
};

};

#endif
