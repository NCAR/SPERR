#ifndef SPECK3D_H
#define SPECK3D_H

#include "SPECK_Storage.h"

#include <limits>

namespace speck {

//
// Auxiliary class to hold a 3D SPECK Set
//
class SPECKSet3D {
public:
    //
    // Member data
    //
    uint32_t start_x  = 0;
    uint32_t start_y  = 0;
    uint32_t start_z  = 0;
    uint32_t length_x = 0;
    uint32_t length_y = 0;
    uint32_t length_z = 0;
    // which partition level is this set at (starting from zero, in all 3 directions).
    // This data member is the sum of all 3 partition levels.
    uint16_t     part_level = 0;
    Significance signif     = Significance::Insig;
    SetType      type       = SetType::TypeS; // Only used to indicate garbage status

public:
    //
    // Member functions
    //
    auto is_pixel() const -> bool;
    auto is_empty() const -> bool;
};

//
// Main SPECK3D class
//
class SPECK3D : public SPECK_Storage {

public:

    // trivial input
    void set_dims(size_t, size_t, size_t); // Accepts volume dimension
    void set_max_coeff_bits(int32_t);

    // trivial output
    void get_dims( size_t&, size_t&, size_t& ) const;

    // How many bits does speck process?
    // If set to zero during decoding, then all bits in the bitstream will be processed.
    void set_bit_budget(size_t);           

#ifdef QZ_TERM
    //
    // Notes for QZ_TERM mode:
    // It changes the behavior of encoding, so encoding terminates at a particular
    // quantization level (2^lev).
    // It does NOT change the behavior of decoding, though.
    //
    void set_quantization_term_level( int32_t lev );
    auto get_num_of_bits() const -> size_t;
#endif

    // core operations
    auto encode() -> RTNType;
    auto decode() -> RTNType;
    auto get_encoded_bitstream() const -> std::pair<buffer_type_uint8, size_t> override;
    auto parse_encoded_bitstream( const void*, size_t ) -> RTNType override;

private:
    //
    // Note: for methods returning an integer, 0 means normal execution, and
    // 1 means bit budget met.
    //
    auto m_ready_to_encode() const  -> bool;
    auto m_ready_to_decode() const  -> bool;
    void m_clean_LIS(); // Clean garbage sets from m_LIS if too much garbage exists.
    void m_initialize_sets_lists();
    auto m_sorting_pass_encode()    -> RTNType;
    auto m_sorting_pass_decode()    -> RTNType;
    auto m_refinement_pass_encode() -> RTNType;
    auto m_refinement_pass_decode() -> RTNType;

    // For the following 5 methods, indices are used to locate which set to process from m_LIS,
    auto m_process_S_encode(size_t idx1, size_t idx2) -> RTNType;
    auto m_process_S_decode(size_t idx1, size_t idx2) -> RTNType;
    auto m_process_P_encode(size_t idx) -> RTNType; // Similar to process_S, but for pixels.
    auto m_process_P_decode(size_t idx) -> RTNType; // Similar to process_S, but for pixels.
    auto m_code_S(size_t idx1, size_t idx2) -> RTNType;

    // Divide a SPECKSet3D into 8, 4, or 2 smaller subsets.
    auto m_partition_S_XYZ(const SPECKSet3D&) const -> std::array<SPECKSet3D, 8>;
    auto m_partition_S_XY (const SPECKSet3D&) const -> std::array<SPECKSet3D, 4>;
    auto m_partition_S_Z  (const SPECKSet3D&) const -> std::array<SPECKSet3D, 2>;

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

    // Now we use a vector of indices to serve the same funcationality of the last LIS,
    // which would contain all insignificant pixels.
    vector_size_t m_LIP;      // List of insignificant pixels.
    const size_t         m_LIP_garbage_val = std::numeric_limits<size_t>::max();

    // List of significant pixels (recorded as locations).
    vector_size_t m_LSP_new; // Ones newly identified as significant
    vector_size_t m_LSP_old; // Ones previously identified as significant

    vector_bool   m_sign_array;

    // Significance Map and a mask that's used in refinement subroutine.
    // A flag is used to label if the map and mask are enabled. They're 
    //   enabled or disabled at the same time.
    vector_bool     m_sig_map;
    vector_uint8_t  m_refinement_mask;
    bool            m_sig_map_enabled = false;
    

#ifdef USE_PMR
    std::pmr::vector<std::pmr::vector<SPECKSet3D>>  m_LIS;
#else
    std::vector<std::vector<SPECKSet3D>>            m_LIS;
#endif

#ifdef QZ_TERM
    int32_t m_qz_term_lev   = 0;  // At which quantization level does encoding terminate?
#endif

};

};

#endif
