#ifndef SPECK_H
#define SPECK_H

#include <memory>
#include <vector>

namespace speck
{

class SPECK
{
public:
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

};

#endif
