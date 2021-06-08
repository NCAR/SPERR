#include "SPECK_Storage.h"

#include <cassert>
#include <cstring>


template <typename T>
auto speck::SPECK_Storage::copy_data(const T* p, size_t len, size_t dimx, size_t dimy, size_t dimz) 
          -> RTNType
{
    static_assert(std::is_floating_point<T>::value,
                  "!! Only floating point values are supported !!");
    if( len != dimx * dimy * dimz )
        return RTNType::DimMismatch;

    // If `m_coeff_buf` is empty, or having a different length, we need to allocate memory!
    m_coeff_buf.resize( len );
    std::copy( p, p + len, m_coeff_buf.begin() );

    m_dim_x = dimx;
    m_dim_y = dimy;
    m_dim_z = dimz;

    return RTNType::Good;
}
template auto speck::SPECK_Storage::copy_data(const double*, size_t, size_t, size_t, size_t) -> RTNType;
template auto speck::SPECK_Storage::copy_data(const float*,  size_t, size_t, size_t, size_t) -> RTNType;


auto speck::SPECK_Storage::take_data(vecd_type&& coeffs, size_t dimx, size_t dimy, size_t dimz) 
                           -> RTNType
{
    if( coeffs.size() != dimx * dimy * dimz )
        return RTNType::DimMismatch;

    m_coeff_buf = std::move(coeffs);
    m_dim_x     = dimx;
    m_dim_y     = dimy;
    m_dim_z     = dimz;

    return RTNType::Good;
}


auto speck::SPECK_Storage::release_data() -> vecd_type
{
    m_dim_x = 0;
    m_dim_y = 0;
    m_dim_z = 0;
    return std::move(m_coeff_buf);
}
auto speck::SPECK_Storage::view_data() const -> const vecd_type&
{
    return m_coeff_buf;
}
auto speck::SPECK_Storage::get_dims() const -> std::array<size_t, 3>
{
    return {m_dim_x, m_dim_y, m_dim_z};
}


auto speck::SPECK_Storage::get_encoded_bitstream() const -> vec8_type
{
    // Header definition:
    // dim_x,     dim_y,     dim_z,     max_coeff_bits,  qz_term_lev,  bitstream_len (in byte)
    // uint32_t,  uint32_t,  uint32_t,  int16_t,         int16_t,      uint64_t

    uint32_t dims[3] { uint32_t(m_dim_x), uint32_t(m_dim_y), uint32_t(m_dim_z) };
    assert( m_bit_buffer.size() % 8 == 0 );
    const uint64_t bit_in_byte = m_bit_buffer.size() / 8;
    const size_t total_size = m_header_size + bit_in_byte;
    auto tmp_buf = std::vector<uint8_t>( total_size );
    auto* const ptr = tmp_buf.data();

    // Fill header 
    size_t pos = 0;
    std::memcpy(ptr, dims, sizeof(dims));
    pos += sizeof(dims);
    int16_t max_bits = int16_t(m_max_coeff_bits);   // int16_t is big enough
    std::memcpy(ptr + pos, &max_bits, sizeof(max_bits));
    pos += sizeof(max_bits);
    int16_t qz_term = int16_t(m_qz_term_lev);       // int16_t is big enough
    std::memcpy(ptr + pos, &qz_term, sizeof(qz_term));
    pos += sizeof(qz_term);
    std::memcpy(ptr + pos, &bit_in_byte, sizeof(bit_in_byte));
    pos += sizeof(bit_in_byte);
    assert( pos == m_header_size );

    // Assemble the bitstream into bytes
    auto rtn = speck::pack_booleans( tmp_buf, m_bit_buffer, pos );
    if( rtn != RTNType::Good )
        return std::vector<uint8_t>(0);
    else
        return std::move(tmp_buf);
}


auto speck::SPECK_Storage::parse_encoded_bitstream( const void* comp_buf, size_t comp_size) -> RTNType
{
    // The buffer passed in is supposed to consist a header and then a compacted bitstream,
    // just like what was returned by `get_encoded_bitstream()`.
    // Note: header definition is documented in get_encoded_bitstream().

    const uint8_t* const ptr = static_cast<const uint8_t*>( comp_buf );

    // Parse the header
    uint32_t dims[3] = {0, 0, 0};
    size_t   pos = 0;
    std::memcpy(dims, ptr, sizeof(dims));
    pos += sizeof(dims);
    int16_t max_bits;
    std::memcpy(&max_bits, ptr + pos, sizeof(max_bits));
    pos += sizeof(max_bits);
    m_max_coeff_bits = max_bits;
    int16_t qz_term;
    std::memcpy(&qz_term, ptr + pos, sizeof(qz_term));
    pos += sizeof(qz_term);
    m_qz_term_lev = qz_term;
    uint64_t bit_in_byte;
    std::memcpy(&bit_in_byte, ptr + pos, sizeof(bit_in_byte));
    pos += sizeof(bit_in_byte);

    // Unpack bits
    auto rtn = speck::unpack_booleans( m_bit_buffer, comp_buf, comp_size, pos );
    if( rtn != RTNType::Good )
        return rtn;

    m_dim_x = dims[0]; 
    m_dim_y = dims[1]; 
    m_dim_z = dims[2];
    m_coeff_buf.resize(m_dim_x * m_dim_y * m_dim_z );

    return RTNType::Good;
}


auto speck::SPECK_Storage::get_speck_stream_size( const void* buf ) const -> uint64_t
{
    // Given the header definition in `get_encoded_bitstream()`, directly
    // go retrieve the value stored in the last 8 bytes of the header
    const uint8_t* const ptr = static_cast<const uint8_t*>( buf );
    uint64_t bit_in_byte;
    std::memcpy(&bit_in_byte, ptr + m_header_size - 8, sizeof(bit_in_byte));

    return (m_header_size + size_t(bit_in_byte));
}


auto speck::SPECK_Storage::get_speck_stream_dims( const void* buf ) const 
            -> std::array<size_t, 3>
{
    // Given the header definition in `get_encoded_bitstream()`, directly
    // go retrieve the value stored in byte 0-12.
    auto dims = std::array<uint32_t, 3>();
    std::memcpy(dims.data(), buf, sizeof(dims));

    return {dims[0], dims[1], dims[2]};
}

