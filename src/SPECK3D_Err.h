/*
 * This class implements the error-bound SPECK3D add-on.
 */

#ifndef SPECK3D_ERR_H
#define SPECK3D_ERR_H

#include "speck_helper.h"

namespace speck {

//
// Helper enum class that represents the status of a pixel.
// Every pixel starts as Insignificant. It is moved to LSP as a NewlySignificant Pixel.
// In the next iteration, it will be a Significant pixel.
//
enum class PixelType : char {
    Insig,
    NewlySig,
    Sig
};

class SPECK3D_Err {
public:
    //
    // Trival input/output functions
    //
    // Input
    // Reserve space for a certain number of outliers. This is optional though.
    void reserve(size_t);

    // Note on the outliers: each one must live at a unique location.
    void add_outlier(uint32_t, uint32_t, uint32_t, float); // add a single outlier.
    void add_outlier_list(std::vector<Outlier>);           // add an entire list of outliers
    void set_dims(size_t, size_t, size_t);                 // set the volume dimensions
    void set_tolerance(float);                             // set error tolerance

    // Output
    auto release_outliers() -> std::vector<Outlier>;// Release ownership of decoded outliers
    auto get_num_of_outliers() const -> size_t;     // How many outliers are decided?
    // This method does NOT perform range check. Seg fault will occur if idx is out of bound.
    auto get_ith_outlier(size_t) const -> Outlier;  // Return a single outlier.  

    // Action methods
    // Returns 0 upon success, 1 upon failure.
    auto encode() -> int;
    auto decode() -> int;

private:
    //
    // Private methods
    //
    void m_initialize_LIS();
    void m_clean_LIS();
    auto m_decide_significance(const SPECKSet3D&) const -> bool;
    auto m_ready_to_encode() const -> bool;
    auto m_ready_to_decode() const -> bool;

    // For the following encoding methods that return a boolean, 
    // True means that all outliers are refined to be within the tolerance
    // False means otherwise.
    auto m_process_S_encoding(size_t, size_t) -> bool;
    auto m_code_S_encoding(size_t, size_t) -> bool;
    auto m_sorting_pass() -> bool;                  // Used in both encoding and decoding
    auto m_refinement_Sig() -> bool;                // Used in encoding only
    auto m_refinement_NewlySig( size_t ) -> bool;   // Used in encoding only

    // For the following decoding methods that return a boolean, 
    // True means that all bits are processed and decoding finishes,
    // False means otherwise.
    auto m_process_S_decoding(size_t, size_t) -> bool;
    auto m_code_S_decoding(size_t, size_t) -> bool;
    auto m_refinement_decoding() -> bool;

    //
    // Private data members
    //
    // Properties that do not change
    size_t  m_dim_x          = 0; // 3D volume dims
    size_t  m_dim_y          = 0;
    size_t  m_dim_z          = 0;
    float   m_tolerance      = 0.0f; // Gamma in the algorithm description
    bool    m_encode_mode    = true; // Encode (true) or Decode (false) mode?
    int32_t m_max_coeff_bits = 0;    // = log2(max_coefficient)

    // Global variables that change and facilitate the process.
    size_t m_outlier_cnt = 0;   // How many data points are still exceeding the tolerance?
    float  m_threshold   = 0.0; // Threshold that's used for quantization
    size_t m_bit_idx     = 0;   // Used for decode. Which bit we're at?

    std::vector<Outlier> m_LOS;   // List of OutlierS. This list is not altered by encoding.
    std::vector<float>   m_q;     // Q list in the algorithm description. This list is
                                  // constantly refined by the refinement pass.
    std::vector<float> m_err_hat; // err_hat list in the algorithm description. This list
                                  // contains the values that would be reconstructed.

    // Records the type of every pixel. It also essentially serves the functionality of LSP.
    std::vector<PixelType>               m_pixel_types;
    std::vector<bool>                    m_bit_buffer;
    std::vector<std::vector<SPECKSet3D>> m_LIS;
    std::vector<size_t>                  m_LIS_garbage_cnt;
};

};

#endif
