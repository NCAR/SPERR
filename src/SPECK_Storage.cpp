#include "SPECK_Storage.h"

#include <cassert>
#include <cstring>
#include <cstdio>


#ifdef USE_PMR
speck::SPECK_Storage::SPECK_Storage()
                    : m_bit_buffer( std::pmr::polymorphic_allocator<bool>(&m_pool) )
{
    // Keep a copy of whatever the current system default memory resource is.
    // That's because I'm not sure how long the std::pmr::set_default_resource() call 
    //   will take effect, but apparently it's longer than the lifespan of this object.
    //   So to not mess up other pmr objects, let's restore the default resource.
    // It cannot be declared const because of how it's used in std::pmr::get_default_resource(),
    //   so let's try to not touch it again before the object destruction :)
    m_previous_resource = std::pmr::get_default_resource();
    
    // Set the default resource, so PMR objects created subsequently in this class
    //   (and its subclasses) will use m_pool as the default resource.
    std::pmr::set_default_resource( &m_pool );

    // Note that m_bit_buffer was created before this default is set,
    //   it was constructed with an explicit allocator.
}

speck::SPECK_Storage::~SPECK_Storage()
{
    // Restore the previous resource before this object was created.
    std::pmr::set_default_resource( m_previous_resource );
}
#endif


template <typename T>
void speck::SPECK_Storage::copy_data(const T* p, size_t len)
{
    static_assert(std::is_floating_point<T>::value,
                  "!! Only floating point values are supported !!");

    assert(len > 0);
    assert(m_coeff_len == 0 || m_coeff_len == len);
    m_coeff_len = len;
    m_coeff_buf = std::make_unique<double[]>(len);
    std::copy( p, p + len, speck::begin(m_coeff_buf) );
}
template void speck::SPECK_Storage::copy_data(const double*, size_t);
template void speck::SPECK_Storage::copy_data(const float*, size_t);

void speck::SPECK_Storage::take_data(buffer_type_d coeffs, size_t len)
{
    assert(len > 0);
    assert(m_coeff_len == 0 || m_coeff_len == len);
    m_coeff_len = len;
    m_coeff_buf = std::move(coeffs);
}

auto speck::SPECK_Storage::get_read_only_data() const -> std::pair<const buffer_type_d&, size_t>
{
    return std::make_pair(std::cref(m_coeff_buf), m_coeff_len);
    // The syntax below would also work, but the one above better expresses the intent.
    //return {m_coeff_buf, m_coeff_len};
}

auto speck::SPECK_Storage::release_data() -> std::pair<buffer_type_d, size_t>
{
    const auto tmp = m_coeff_len;
    m_coeff_len = 0;
    return {std::move(m_coeff_buf), tmp};
}

void speck::SPECK_Storage::set_image_mean(double mean)
{
    m_image_mean = mean;
}
auto speck::SPECK_Storage::get_image_mean() const -> double
{
    return m_image_mean;
}
auto speck::SPECK_Storage::get_bit_buffer_size() const -> size_t
{
    return m_bit_buffer.size();
}
auto speck::SPECK_Storage::get_dims() const -> std::array<size_t, 3>
{
    return {m_dim_x, m_dim_y, m_dim_z};
}




auto speck::SPECK_Storage::get_encoded_bitstream() const -> smart_buffer_uint8
{
    // Header definition:
    // dim_x,     dim_y,     dim_z,     image_mean,  max_coeff_bits,  bitstream_len (in byte)
    // uint32_t,  uint32_t,  uint32_t,  double       int32_t,         uint64_t

    uint32_t dims[3] { uint32_t(m_dim_x), uint32_t(m_dim_y), uint32_t(m_dim_z) };
    assert( m_bit_buffer.size() % 8 == 0 );
    const uint64_t bit_in_byte = m_bit_buffer.size() / 8;
    const size_t total_size = m_header_size + bit_in_byte;
    auto tmp_buf = std::make_unique<uint8_t[]>( total_size );
    auto* const ptr = tmp_buf.get();

    // Fill header 
    size_t pos = 0;
    std::memcpy(ptr, dims, sizeof(dims));
    pos += sizeof(dims);
    std::memcpy(ptr + pos, &m_image_mean, sizeof(m_image_mean));
    pos += sizeof(m_image_mean);
    std::memcpy(ptr + pos, &m_max_coeff_bits, sizeof(m_max_coeff_bits));
    pos += sizeof(m_max_coeff_bits);
    std::memcpy(ptr + pos, &bit_in_byte, sizeof(bit_in_byte));
    pos += sizeof(bit_in_byte);
    assert( pos == m_header_size );

    // Assemble the bitstream into bytes
    auto rtn = speck::pack_booleans( tmp_buf, m_bit_buffer, pos );
    if( rtn != RTNType::Good )
        return {nullptr, 0};
    else
        return {std::move(tmp_buf), total_size};
}


auto speck::SPECK_Storage::parse_encoded_bitstream( const void* comp_buf, size_t comp_size) 
            -> RTNType
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
    std::memcpy(&m_image_mean, ptr + pos, sizeof(m_image_mean));
    pos += sizeof(m_image_mean);
    std::memcpy(&m_max_coeff_bits, ptr + pos, sizeof(m_max_coeff_bits));
    pos += sizeof(m_max_coeff_bits);
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
    m_coeff_len = m_dim_x * m_dim_y * m_dim_z;

    return RTNType::Good;
}


auto speck::SPECK_Storage::get_speck_stream_size( const void* buf ) const -> uint64_t
{
    // Given the header definition in `get_encoded_bitstream()`, directly
    // go retrieve the value stored in byte 24-31.
    const uint8_t* const ptr = static_cast<const uint8_t*>( buf );
    uint64_t bit_in_byte;
    std::memcpy(&bit_in_byte, ptr + 24, sizeof(bit_in_byte));

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

