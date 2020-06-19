/*
 * This class implements the error-bound SPECK3D add-on.
 */

#ifndef SPECK3D_ERR_H
#define SPECK3D_ERR_H

#include "speck_helper.h"

namespace speck {

class SPECK3D_Err {
public:
    //
    // Trival input/output functions
    //
    // Input
    // Reserve space for a certain number of outliers. This is optional though.
    void reserve(size_t);                       
    void add_outlier(uint32_t, uint32_t, uint32_t, float); // add a single outlier.
    void add_outlier_list( std::vector<Outlier> );         // add an entire list of outliers
    void set_dims(size_t, size_t, size_t);                 // set the volume dimensions
    void set_tolerance(float);                             // set error tolerance

    // Output
    //std::vector<Outlier> release_outliers();        // Release ownership of decoded outliers
    //auto get_num_of_outliers() const -> size_t;     // How many outliers are decided?
    //auto get_ith_outlier(size_t) const -> Outlier;  // Return a single outlier

    // Action methods
    auto encode() -> int;

private:
    //
    // Private methods
    //
    void m_initialize_LIS();
    void m_clean_LIS();
    auto m_ready_to_encode() const -> bool;


    //
    // Private data members
    //
    // Properties that do not change
    size_t  m_dim_x          = 0;    // 3D volume dims
    size_t  m_dim_y          = 0;
    size_t  m_dim_z          = 0;
    float   m_tolerance      = 0.01; // Gamma in the algorithm description
    bool    m_encode_mode    = true; // Encode (true) or Decode (false) mode?
    int32_t m_max_coeff_bits = 0;    // = log2(max_coefficient)

    // Global variables that change and facilitate the process.
    size_t  m_outlier_cnt    = 0;    // How many data points are still exceeding the tolerance?
    float   m_threshold      = 0.0;  // Threshold that's used for quantization
    size_t  m_bit_idx        = 0;    // Used for decode. Which bit we're at?

    std::vector<bool>    m_bit_buffer;
    std::vector<Outlier> m_LOS;      // List of OutlierS. This list is not altered by encoding.
    std::vector<float>   m_q;        // Q list in the algorithm description. This list is
                                     // constantly refined by the refinement pass.
    std::vector<float>  m_err_hat;   // err_hat list in the algorithm description. This list
                                     // contains the values that would be reconstructed.
    std::vector<size_t> m_LSP;       // Records locations of significant pixels
    std::vector<bool>   m_LSP_newly; // Records if this pixel is newly significant or not.

    std::vector<std::vector<SPECKSet3D>> m_LIS;
    std::vector<size_t>                  m_LIS_garbage_cnt;
};

};

#endif
