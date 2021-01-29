//
// This class implements the SPECK encoder for errors, dubbed as SPERR.
//

#ifndef SPERR_H
#define SPERR_H

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

//
// Auxiliary struct to hold represent an outlier
//
struct Outlier {
    size_t location = 0;
    double error    = 0.0;

    Outlier() = default;
    Outlier( size_t, double);
};


class SPERR
{
public:
    // Input
    //
    // Important note on the outliers: each one must live at a unique location,
    // and each error value must be greater than the tolerance.
    void add_outlier(size_t, double);            // add a single outlier.
                                                 // Does not affect existing outliers.
    void use_outlier_list(std::vector<Outlier>); // use a given list of outliers.
                                                 // Existing outliers are erased.
    void set_length(uint64_t);                   // set 1D array length
    void set_tolerance(double);                  // set error tolerance (Must be positive)

    // Output
    auto release_outliers() -> std::vector<Outlier>; // Release ownership of decoded outliers
    auto ith_outlier(size_t) const -> Outlier;       // Get a single outlier (No range check here!)
    auto num_of_outliers() const -> size_t;          // How many outliers are decoded?
    auto num_of_bits() const -> size_t;              // How many bits are generated?
    auto max_coeff_bits() const -> int32_t;          // Will be used when decoding.

    // Note that this class isn't performance critical, so don't bother using PMR containers.
    auto get_encoded_bitstream() const -> smart_buffer_uint8;
    auto parse_encoded_bitstream( const void*, size_t ) -> RTNType;

    // Action methods
    auto encode() -> RTNType;
    auto decode() -> RTNType;


private:
    //
    // Private methods
    //
    auto m_part_set(const SPECKSet1D&) const -> std::array<SPECKSet1D, 2>;
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
    auto m_refinement_pass_encoding() -> bool;
    auto m_refinement_new_SP(size_t) -> bool;   // Refine a new Significant Pixel (encoding only).

    // For the following decoding methods that return a boolean,
    // True means that all bits are processed and decoding finishes,
    // False means otherwise.
    auto m_process_S_decoding(size_t, size_t) -> bool;
    auto m_refinement_decoding() -> bool;

    //
    // Private data members
    //
    uint64_t     m_total_len      = 0;    // 1D array length
    double       m_tolerance      = 0.0;  // Error tolerance.
    int32_t      m_max_coeff_bits = 0;    // = log2(max_coefficient)
    const size_t m_header_size    = 20;   // in bytes

    size_t m_outlier_cnt = 0;   // How many data points are still exceeding the tolerance?
    double m_threshold   = 0.0; // Threshold that's used for quantization 
    bool   m_encode_mode = true;// Encode (true) or Decode (false) mode?
    size_t m_bit_idx     = 0;   // decoding only. Which bit we're at?
    size_t m_LOS_size    = 0;   // decoding only. Size of `m_LOS` at the end of an iteration.

    std::vector<bool>    m_bit_buffer;
    std::vector<Outlier> m_LOS;     // List of OutlierS. This list is not altered when encoding,
                                    // but constantly updated when decoding.
    std::vector<double>  m_q;       // encoding only. This list is refined in the refinement pass. 
    std::vector<double>  m_err_hat; // encoding only. This list contains values that
                                    // would be reconstructed.

    std::vector<bool>    m_recovered_signs; // decoding only
    std::vector<size_t>  m_LSP_new;         // encoding only
    std::vector<size_t>  m_LSP_old;         // encoding only

    std::vector<std::vector<SPECKSet1D>> m_LIS;
};

};

#endif
