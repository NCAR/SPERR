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
    uint16_t part_level = 0;
    SigType  signif     = SigType::Insig;
    SetType  type       = SetType::TypeS; // Only used to indicate garbage status

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

    // How many bits does speck process (for encoding and decoding).
    // If set to zero during decoding, then all bits in the bitstream will be processed.
    void set_bit_budget(size_t);           

#ifdef QZ_TERM
    // Notes for QZ_TERM mode:
    // It changes the behavior of encoding, so encoding terminates at a particular
    // quantization level (2^lev).
    // It does NOT change the behavior of decoding, though.
    //
    void set_quantization_term_level( int32_t lev );
#endif

    // core operations
    auto encode() -> RTNType;
    auto decode() -> RTNType;
    
private:
    auto m_ready_to_encode() const  -> bool;
    auto m_ready_to_decode() const  -> bool;
    void m_clean_LIS(); // Clean garbage sets from m_LIS if too much garbage exists.
    void m_initialize_sets_lists();
    auto m_sorting_pass_encode()    -> RTNType;
    auto m_sorting_pass_decode()    -> RTNType;

    // For the following 5 methods, indices are used to locate which set to process from m_LIS,
    // Note that when process_S or process_P is called from a code_S routine, code_S will
    // pass in a counter to record how many subsets are discovered significant already.
    // That counter, however, doesn't do anything if process_S or process_P is called from
    // the sorting pass.
    auto m_process_S_encode(size_t idx1, size_t idx2, SigType, size_t& counter, bool) -> RTNType;
    auto m_code_S_encode(   size_t idx1, size_t idx2, std::array<SigType, 8>) -> RTNType;
    auto m_process_P_encode(size_t idx,  SigType, size_t& counter, bool) -> RTNType;

    auto m_process_S_decode(size_t idx1, size_t idx2, size_t& counter, bool) -> RTNType;
    auto m_code_S_decode(   size_t idx1, size_t idx2) -> RTNType;
    auto m_process_P_decode(size_t idx,  size_t& counter, bool) -> RTNType;

    // Divide a SPECKSet3D into 8, 4, or 2 smaller subsets.
    auto m_partition_S_XYZ(const SPECKSet3D&) const -> std::array<SPECKSet3D, 8>;
    auto m_partition_S_XY (const SPECKSet3D&) const -> std::array<SPECKSet3D, 4>;
    auto m_partition_S_Z  (const SPECKSet3D&) const -> std::array<SPECKSet3D, 2>;

    // Decide if a set is significant or not
    auto m_decide_significance(const SPECKSet3D&, std::array<uint32_t, 3>& xyz) const -> SigType;

#ifdef QZ_TERM
    // Quantize a pixel to the specified m_qz_term_lev.
    void m_quantize_P_encode( size_t idx );
    void m_quantize_P_decode( size_t idx );
#else
    // Quantize pixels bitplane by bitplane.
    auto m_refinement_pass_encode() -> RTNType;
    auto m_refinement_pass_decode() -> RTNType;
#endif

    //
    // Private data members
    //
    size_t  m_budget         = 0;   // What's the budget for num of bits?
    size_t  m_bit_idx        = 0;   // Used for decode. Which bit we're at?
    bool    m_encode_mode    = true; // Encode (true) or Decode (false) mode?

    const size_t        m_u64_garbage_val = std::numeric_limits<size_t>::max();
    const uint8_t       m_false   = 0;
    const uint8_t       m_true    = 1;
    const uint8_t       m_discard = 2;
    std::vector<bool>   m_sign_array;

    std::vector<std::vector<SPECKSet3D>>  m_LIS;
    std::vector<size_t>                   m_LIP;

#ifndef QZ_TERM
    std::vector<size_t>                   m_LSP_new;
    std::vector<size_t>                   m_LSP_old;
#endif

    int32_t                 m_threshold_idx = 0;
    std::array<double, 64>  m_threshold_arr = {0.0};

};

};

#endif
