#ifndef SPECK_STORAGE_H
#define SPECK_STORAGE_H

/* 
 * This class implements mechanisms and interfaces to help 2D and 3D SPECK classes 
 * manage storage of coefficients and bitstreams.
 */

#include "speck_helper.h"
#include <vector>

namespace speck {

class SPECK_Storage {

public:
    // memory management: input
    void take_coeffs(buffer_type_d, size_t);       // Take ownership of the incoming buffer
    void take_coeffs(buffer_type_f, size_t);       // Take ownership of the incoming buffer
    template <typename T>
    void copy_coeffs(const T*, size_t len);        // Make a copy of the incoming data.

    // memory management: output
    auto get_read_only_coeffs() const -> const buffer_type_d&;
    auto release_coeffs_double() -> buffer_type_d;  // Others take ownership of the data
    auto release_coeffs_float() -> buffer_type_f;   // Others take ownership of the data

    // file operations
    virtual auto write_to_disk(const std::string& filename) const -> int = 0;
    virtual auto read_from_disk(const std::string& filename) -> int      = 0;

    // Get the compressed form in the memory. 
    // The returned memory block could be written to disk by other programs.
    // This memory block would contain exactly the same content as what is written to disk
    // by write_to_disk() .
    //
    // Note: the caller is supposed to take ownership of the block of memory.
    virtual auto get_compressed_buffer( buffer_type_raw& , size_t& ) const -> int = 0;

    void set_image_mean(double mean);
    auto get_image_mean() const -> double;
    auto get_bit_buffer_size() const -> size_t; // Report the available bits when decoding

protected:
    //
    // Member functions
    //

    // Output to and input from disk
    // They return 0 upon success, and 1 upon failure.
    auto m_read(       void* header, size_t header_size, const char* filename) -> int;

    // Prepare the compressed memory, which includes metadata, header, and the bitstream. 
    // It compacts the bitstream, prepends a header, prepends metadata, and optionally
    //   applies zstd compression.
    // The resulting memory block, out_buf, could be directly written to disk.
    // Any content that's already in out_buf will be destroyed.
    //
    // Note: The caller is supposed to hold ownership of the resulting memory block.
    //
    // Note: Here we use raw pointer for header because we want to address stack addresses.
    auto m_assemble_compressed_buffer( const void* header,       size_t  header_size,
                                       buffer_type_raw& out_buf, size_t& out_size ) const -> int;

    // Parse a compressed buffer, and extract the metadata, header, and bitstream from it.
    //
    // Note: `header` should already have memory allocated with `header_size` in size.
    // Note: Here we use raw pointer for header because we want to address stack addresses.
    // Note: Here we use raw pointer for comp_buf because we're accessing memory provided
    //       by others, and others most likely provide a raw pointer. 
    auto m_disassemble_compressed_buffer( void* header,         size_t header_size, 
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
