//
// This class implements the error-bound SPECK add-on.
//

#ifndef SPECK_ERR_H
#define SPECK_ERR_H

#include "speck_helper.h"

namespace speck
{

//
// Auxiliary class to hold a 1D SPECK Set
//
class SPECKSet1D {
public:
    size_t        start      = 0;
    size_t        length     = 0;
    uint32_t      part_level = 0;
    SetType       type       = SetType::TypeS;    // only to indicate if it's garbage

    SPECKSet1D() = default;
    SPECKSet1D( size_t start, size_t len, uint32_t part_lev );
};
using TwoSets = std::array<SPECKSet1D, 2>;

//
// Auxiliary struct to hold represent an outlier
//
struct Outlier {
    size_t location = 0;
    double error    = 0.0;

    Outlier() = default;
    Outlier( size_t, double);
};


class SPECK_Err
{
public:
    //
    // Trival input/output
    //
    // Important note on the outliers: each one must live at a unique location.
    //
    // Input
    void reserve(size_t);                        // Optionally pre-allocate space. ?? What number ??
    void add_outlier(size_t, double);            // add a single outlier.
    void use_outlier_list(std::vector<Outlier>); // add an entire list of outliers
    void set_length(size_t);                     // set 1D array length
    void set_tolerance(double);                  // set error tolerance

    // Output
    auto release_outliers() -> std::vector<Outlier>; // Release ownership of decoded outliers
    auto get_num_of_outliers() const -> size_t;      // How many outliers are decoded?
    auto get_ith_outlier(size_t) const -> Outlier;   // Get a single outlier (No range check here!)
    auto num_of_bits() const -> size_t;              // How many bits are generated?

    // Action methods
    // Returns 0 upon success, 1 upon failure.
    auto encode() -> RTNType;
    auto decode() -> RTNType;

private:
    //
    // Private methods
    //
    auto m_part_set(const SPECKSet1D&) const -> TwoSets;
    void m_initialize_LIS();
    void m_clean_LIS();
    auto m_ready_to_encode() const -> bool;
    auto m_ready_to_decode() const -> bool;

    // If the set to be decided is significant, return a pair containing true and 
    //   the index that makes it significant.
    // If not, return a pair containing false and zero.
    auto m_decide_significance(const SPECKSet1D&) const -> std::pair<bool, size_t>;

    // For the following encoding methods that return a boolean,
    // True means that all outliers are refined to be within the tolerance
    // False means otherwise.
    auto m_process_S_encoding(size_t, size_t) -> bool;
    auto m_code_S(size_t, size_t) -> bool;
    auto m_sorting_pass() -> bool;              // Used in both encoding and decoding
    auto m_refinement_Sig() -> bool;            // Used in encoding only
    auto m_refinement_NewlySig(size_t) -> bool; // Used in encoding only

    // For the following decoding methods that return a boolean,
    // True means that all bits are processed and decoding finishes,
    // False means otherwise.
    auto m_process_S_decoding(size_t, size_t) -> bool;
    auto m_refinement_decoding() -> bool;

    //
    // Private data members
    //
    // Properties that do not change
    size_t  m_total_len      = 0;    // 1D array length
    double  m_tolerance      = 0.0;  // Gamma in the algorithm description
    bool    m_encode_mode    = true; // Encode (true) or Decode (false) mode?
    int32_t m_max_coeff_bits = 0;    // = log2(max_coefficient)

    // Global variables that change and facilitate the process.
    size_t m_outlier_cnt = 0;   // How many data points are still exceeding the tolerance?
    double m_threshold   = 0.0; // Threshold that's used for quantization 
    size_t m_bit_idx     = 0;   // Used for decode. Which bit we're at?

    std::vector<Outlier> m_LOS;    // List of OutlierS. This list is not altered by encoding.
    std::vector<double>  m_q;      // Q list in the algorithm description. This list is
                                   // constantly refined by the refinement pass.
    std::vector<double> m_err_hat; // err_hat list in the algorithm description. This list
                                   // contains the values that would be reconstructed.

    std::vector<Significance>            m_pixel_types; // Records the type of every pixel.
    std::vector<bool>                    m_bit_buffer;
    std::vector<size_t>                  m_LSP;
    std::vector<std::vector<SPECKSet1D>> m_LIS;
};

};

#endif
