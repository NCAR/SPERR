/*
 * This class implements the error-bound SPECK add-on.
 */

#ifndef SPECK_ERR_H
#define SPECK_ERR_H

#include "speck_helper.h"

namespace speck {

class SPECKSet1D {
public:
    size_t        start      = 0;
    size_t        length     = 0;
    uint32_t      part_level = 0;
    Significance  signif     = Significance::Insig;
    SetType       type       = SetType::TypeS;    // only to indicate if it's garbage

    SPECKSet1D() = default;
    SPECKSet1D( size_t, size_t, uint32_t );
};

using TwoSets = std::array<SPECKSet1D, 2>;
using Outlier = std::pair<uint32_t, float>;


class SPECK_Err {
public:
    //
    // Trival input/output functions
    //
    // Input
    // Reserve space for a certain number of outliers. This is optional though.
    void reserve(size_t);

    // Note on the outliers: each one must live at a unique location.
    void add_outlier(uint32_t, float);              // add a single outlier.
    void add_outlier_list(std::vector<Outlier>);    // add an entire list of outliers
    void set_length(size_t);                        // set 1D array length
    void set_tolerance(float);                      // set error tolerance

    // Output
    auto release_outliers() -> std::vector<Outlier>;// Release ownership of decoded outliers
    auto get_num_of_outliers() const -> size_t;     // How many outliers are decoded?
    // This method does NOT perform range check. Seg fault will occur if idx is out of bound.
    auto get_ith_outlier(size_t) const -> Outlier;  // Return a single outlier.  
    auto num_of_bits() const -> size_t;             // How many bits are generated?

    // Action methods
    // Returns 0 upon success, 1 upon failure.
    auto encode() -> int;
    auto decode() -> int;

private:
    //
    // Private methods
    //
    auto m_part_set( const SPECKSet1D& ) const -> TwoSets;
    void m_initialize_LIS();
    void m_clean_LIS();
    auto m_ready_to_encode() const -> bool;
    auto m_ready_to_decode() const -> bool;
    // If the set to be decided is significant, return the index that makes it significant.
    // If not, return -1
    auto m_decide_significance(const SPECKSet1D&) const -> int64_t;

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
    size_t  m_total_len      = 0;    // 1D array length
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

    std::vector<Significance>            m_pixel_types; // Records the type of every pixel.
    std::vector<bool>                    m_bit_buffer;
    std::vector<size_t>                  m_LSP;
    std::vector<std::vector<SPECKSet1D>> m_LIS;
    std::vector<size_t>                  m_LIS_garbage_cnt;
};

};

#endif
