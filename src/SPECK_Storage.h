#ifndef SPECK_STORAGE_H
#define SPECK_STORAGE_H

/* 
 * This class implements mechanisms and interfaces to help 2D and 3D SPECK classes 
 * manage storage of coefficients and bitstreams.
 */

#include <vector>
#include <string>
#include "speck_helper.h"

namespace speck {

class SPECK_Storage {

public:
    // memory management: input
    void take_coeffs(buffer_type_d, size_t);       // Take ownership of the incoming buffer
    template <typename T>
    void copy_coeffs(const T*, size_t len);        // Make a copy of the incoming data.

    // memory management: output
    auto get_read_only_coeffs() const -> const buffer_type_d&; // Keep ownership.
    auto release_coeffs() -> buffer_type_d;                    // Transfer ownership

    // Get the encoded bitstream.
    // The returned memory block could be written to disk by other programs.
    //
    // Note: the caller does NOT need to allocate memory for the returned buffer. 
    //       However, it is supposed to take ownership of the returned block of memory.
    virtual auto get_encoded_bitstream( buffer_type_raw& , size_t& ) const -> int = 0;

    // Prepare internal states for a decompression operation from an encoded bitstream
    //
    // Note: it takes a raw pointer because it accesses memory provided by others,
    //       and others most likely provide a raw pointer.
    virtual auto read_encoded_bitstream( const void*, size_t ) -> int = 0;

    void set_image_mean(double mean);
    auto get_image_mean() const -> double;
    auto get_bit_buffer_size() const -> size_t; // Report the available bits when decoding

protected:
    //
    // Member functions
    //

    // Prepare the compressed memory, which includes metadata, header, and the bitstream. 
    // It compacts the bitstream, prepends a header, prepends metadata, and optionally
    //   applies zstd compression.
    // The resulting memory block, out_buf, could be directly written to disk.
    // Any content that's already in out_buf will be destroyed.
    //
    // Note: The caller is supposed to hold ownership of the resulting memory block.
    //
    // Note: Here we use raw pointer for header because we want to address stack addresses.
    auto m_assemble_encoded_bitstream( const void* header,       size_t  header_size,
                                       buffer_type_raw& out_buf, size_t& out_size ) const -> int;

    // Parse a compressed buffer, and extract the metadata, header, and bitstream from it.
    //
    // Note: `header` should already have memory allocated with `header_size` in size.
    // Note: Here we use raw pointer for header because we want to address stack addresses.
    // Note: Here we use raw pointer for comp_buf because we're accessing memory provided
    //       by others, and others most likely provide a raw pointer. 
    auto m_disassemble_encoded_bitstream( void* header,         size_t header_size, 
                                          const void* comp_buf, size_t comp_size ) -> int;

    //
    // Member variables
    //
    double            m_image_mean = 0.0;    // Mean of the original data. Used by DWT/IDWT.
    size_t            m_coeff_len  = 0;
    std::vector<bool> m_bit_buffer;          // Output/Input bit buffer
    buffer_type_d     m_coeff_buf = nullptr; // All coefficients are kept here
};

};

#endif
