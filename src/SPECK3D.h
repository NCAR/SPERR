#ifndef SPECK3D_H
#define SPECK3D_H

#include "SPECK_Storage.h"

namespace speck {

//
// Main SPECK3D class
//
class SPECK3D : public SPECK_Storage {
public:
    // trivial input
    void set_dims(size_t, size_t, size_t); // Accepts volume dimension
    void set_max_coeff_bits(int32_t);

    // How many bits does speck process?
    // If set to zero during decoding, then all bits in the bitstream will be processed.
    void set_bit_budget(size_t);           

#ifdef QZ_TERM
    //
    // Notes for QZ_TERM mode:
    // It changes the behavior of encoding, so encoding terminates at a particular
    // quantization level. It does NOT change the behavior of decoding, though.
    //
    // There are two approaches to specify the quantization level to terminate at.
    // 1) A user specifies how many quantization iterations to perform, by calling
    //    set_quantization_iterations() with a positive value. 
    // 2) A user specifies EXACTLY the quantization level to terminate, by calling
    //    set_quantization_term_level() with a positive or negative value.
    //
    // Internally, the encoding algorithm prioritizes approach 1), and then approach 2).
    //
    void set_quantization_term_level( int32_t );
    void set_quantization_iterations( int32_t );
    auto get_num_of_bits() const -> size_t;
    auto get_quantization_term_level() const -> int32_t;
#endif

    // trivial output
    void get_dims(size_t&, size_t&, size_t&) const;

    // core operations
    auto encode() -> int;
    auto decode() -> int;
    auto write_to_disk(const std::string& filename) const -> int override;
    auto read_from_disk(const std::string& filename) -> int override;

    auto get_compressed_buffer( buffer_type_uc& , size_t& ) const -> int override;

private:
    //
    // Note: for methods returning an integer, 0 means normal execution, and
    // 1 means bit budget met.
    //
    auto m_ready_to_encode() const  -> bool;
    auto m_ready_to_decode() const  -> bool;
    void m_clean_LIS(); // Clean garbage sets from m_LIS if too much garbage exists.
    void m_initialize_sets_lists();
    auto m_sorting_pass_encode()    -> int;
    auto m_sorting_pass_decode()    -> int;
    auto m_refinement_pass_encode() -> int;
    auto m_refinement_pass_decode() -> int;

    // For the following 5 methods, indices are used to locate which set to process from m_LIS,
    auto m_process_S_encode(size_t idx1, size_t idx2) -> int;
    auto m_process_S_decode(size_t idx1, size_t idx2) -> int;
    auto m_process_P_encode(size_t idx) -> int; // Similar to process_S, but for pixels.
    auto m_process_P_decode(size_t idx) -> int; // Similar to process_S, but for pixels.
    auto m_code_S(size_t idx1, size_t idx2) -> int;

    //
    // Private data members
    //
    double  m_threshold      = 0.0; // Threshold that's used for quantization
    size_t  m_budget         = 0;   // What's the budget for num of bits?
    size_t  m_bit_idx        = 0;   // Used for decode. Which bit we're at?
    size_t  m_dim_x          = 0;   // 3D volume dims
    size_t  m_dim_y          = 0;
    size_t  m_dim_z          = 0;
    bool    m_encode_mode    = true; // Encode (true) or Decode (false) mode?
    int32_t m_max_coeff_bits = 0;    // = log2(max_coefficient)

    std::vector<std::vector<SPECKSet3D>> m_LIS;
    std::vector<size_t>                  m_LIS_garbage_cnt;

    std::vector<bool> m_significance_map; // only used when encoding.
    std::vector<bool> m_sign_array;

    std::vector<size_t> m_LSP;       // Records locations of significant pixels
    std::vector<bool>   m_LSP_newly; // Records if this pixel is newly significant or not.

    // Now we use a vector of indices to serve the same funcationality of the last LIS,
    // which would contain all insignificant pixels.
    std::vector<size_t> m_LIP;         // List of insignificant pixels.
    std::vector<bool>   m_LIP_garbage; // If this insignificant pixel is considered garbage.
    size_t              m_LIP_garbage_cnt = 0;

#ifdef QZ_TERM
    int32_t m_qz_term_lev   = 0;  // At which quantization level does encoding terminate?
    int32_t m_qz_iterations = 0;  // How many quantization iterations to perform? 
#endif
};

};

#endif
