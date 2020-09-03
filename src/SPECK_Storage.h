#ifndef SPECK_STORAGE_H
#define SPECK_STORAGE_H

/* 
 * This class implements mechanisms and interfaces to help 2D and 3D SPECK classes 
 * manage storage of coefficients and bitstreams.
 */

#include <vector>
#include <string>
#include <utility> // std::pair<>
#include "speck_helper.h"

namespace speck {

class SPECK_Storage {

public:
    //
    // Memory management: input and output
    // Note: the nature of the data being input and output are wavelet coefficients
    //       either from a wavelet transform (encoding) or from a bitstream (decoding).
    //
    template <typename T>
    void copy_data(const T*, size_t len);        // Make a copy of the incoming data.
    void take_data(buffer_type_d, size_t);       // Take ownership of the incoming buffer
    auto get_read_only_data() const -> std::pair<const buffer_type_d&, size_t>; // Keep ownership
    auto release_data()             -> std::pair<buffer_type_d, size_t>;     // Release ownership

    // Get the encoded bitstream.
    // The returned memory block could be written to disk by other programs.
    //
    virtual auto get_encoded_bitstream() const -> std::pair<buffer_type_raw, size_t> = 0;

    // Prepare internal states for a decompression operation from an encoded bitstream
    //
    // Note: it takes a raw pointer because it accesses memory provided by others,
    //       and others most likely provide a raw pointer.
    virtual auto read_encoded_bitstream( const void*, size_t ) -> RTNType = 0;

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
    auto m_assemble_encoded_bitstream( const void* header, size_t header_size ) const
         -> std::pair<buffer_type_raw, size_t>;

    // Parse a compressed buffer, and extract the metadata, header, and bitstream from it.
    //
    // Note: `header` should already have memory allocated with `header_size` in size.
    // Note: Here we use raw pointer for header because we want to be able to address stack memory.
    // Note: Here we use raw pointer for int_buf because we're accessing memory provided
    //       by others, and others most likely provide a raw pointer. 
    auto m_disassemble_encoded_bitstream( void* header,       size_t header_size, 
                                          const void* in_buf, size_t in_size ) -> RTNType;

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
