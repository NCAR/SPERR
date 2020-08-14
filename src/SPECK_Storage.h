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
    void copy_coeffs(const T&, size_t len);        // Make a copy of the incoming data.
    template <typename T>
    void copy_coeffs(const T*, size_t len);        // Make a copy of the incoming data.
    void take_bitstream(std::vector<bool>&);       // Take ownership of the bitstream.
    void copy_bitstream(const std::vector<bool>&); // Make a copy of the bitstream.

    // memory management: output
    auto get_read_only_bitstream() const -> const std::vector<bool>&;
    auto get_read_only_coeffs() const -> const buffer_type_d&;
    auto release_bitstream() -> std::vector<bool>;  // Give up ownership of the bitstream
    auto release_coeffs_double() -> buffer_type_d;  // Others take ownership of the data
    auto release_coeffs_float() -> buffer_type_f;   // Others take ownership of the data

    // file operations
    virtual auto write_to_disk(const std::string& filename) const -> int = 0;
    virtual auto read_from_disk(const std::string& filename) -> int      = 0;

    void set_image_mean(double mean);
    auto get_image_mean() const -> double;
    auto get_bit_buffer_size() const -> size_t; // Report the available bits when decoding

protected:
    //
    // Member functions
    //

    // Output to and input from disk
    // They return 0 upon success, and 1 upon failure.
    auto m_write(const void* header, size_t header_size, const char* filename) const -> int;
    auto m_read(       void* header, size_t header_size, const char* filename) -> int;

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
