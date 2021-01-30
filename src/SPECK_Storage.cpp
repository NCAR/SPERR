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
    
auto speck::SPECK_Storage::get_speck_stream_size( const void* buf, size_t buf_size) const 
            -> uint64_t
{
    if( buf_size <= m_header_size )
        return 0;

    // Given the header definition in `get_encoded_bitstream()`, directly
    // go retrieve the value stored in byte 24-31.
    const uint8_t* const ptr = static_cast<const uint8_t*>( buf );
    uint64_t bit_in_byte;
    std::memcpy(&bit_in_byte, ptr + 24, sizeof(bit_in_byte));

    return m_header_size + bit_in_byte;
}


#if 0
auto speck::SPECK_Storage::m_assemble_encoded_bitstream( const void* header, 
                                                         size_t header_size ) const
                           -> std::pair<buffer_type_uint8, size_t>
{
    // Sanity check on the size of bit_buffer
    if(m_bit_buffer.size() % 8 != 0)
        return {nullptr, 0};

    // Allocate output buffer
    //
    // Note that there is metadata prepended for each output in the form of two plain bytes:
    // the 1st byte records the current major version of SPECK, and
    // the 2nd byte records 8 booleans, with their meanings documented below:
    // 
    // bool_byte[0]  : if the rest of the stream is zstd compressed.
    // bool_byte[1-7]: undefined.
    //
    const size_t meta_size   = 2;
    const size_t stream_size = m_bit_buffer.size() / 8;
    const size_t total_size  = meta_size + header_size + stream_size;
    auto local_buf           = speck::unique_malloc<uint8_t>( total_size );

    // Fill in metadata as defined above
    local_buf[0] = uint8_t(SPECK_VERSION_MAJOR);
    bool meta_bools[8] = {false, false, false, false, false, false, false, false};

#ifdef USE_ZSTD
    meta_bools[0] = true;
#endif

    speck::pack_8_booleans( local_buf[1], meta_bools );  // pack the bools to the 2nd byte of buf.

    // Copy over header
    std::memcpy(local_buf.get() + meta_size, header, header_size);

    // Pack booleans to buf!
    RTNType rtn = pack_booleans( local_buf, m_bit_buffer, meta_size + header_size );
    if( rtn != RTNType::Good )
        return {nullptr, 0};

#ifdef USE_ZSTD
    const size_t comp_buf_size = ZSTD_compressBound( header_size + stream_size );

    // We prepend metadata to the new buffer, so allocate space accordingly
    auto comp_buf = speck::unique_malloc<uint8_t>( meta_size + comp_buf_size ); 
    std::memcpy( comp_buf.get(), local_buf.get(), meta_size );

    // Use ZSTD to compress `local_buf` and store the result in `comp_buf`.
    const size_t comp_size = ZSTD_compress( comp_buf.get() + meta_size, comp_buf_size, 
                             local_buf.get() + meta_size, header_size + stream_size, 
                             ZSTD_CLEVEL_DEFAULT ); // Just use the default compression level.
    if( ZSTD_isError( comp_size ) )
        return {nullptr, 0};
    else
        return {std::move(comp_buf), meta_size + comp_size};   
        // Note that `comp_buf` could be longer than `meta_size + comp_size`, 
        //      but `meta_size + comp_size` is the length of useful data.
#else
    return {std::move(local_buf), total_size};
#endif
}
#endif


#if 0
auto speck::SPECK_Storage::m_disassemble_encoded_bitstream( void*  header, 
                                                            size_t header_size, 
                                                            const void* comp_buf,
                                                            size_t comp_size) -> RTNType
{
    const size_t meta_size  = 2;    // See m_assemble_encoded_bitstream() for the definition 
                                    // of metadata and meta_size
    // Give an alias to comp_buf so we can do pointer arithmetic
    const uint8_t* const comp_buf_ptr = static_cast<const uint8_t*>(comp_buf);

    // Let's parse the metadata
    uint8_t meta[2];
    std::memcpy( meta, comp_buf, sizeof(meta) );

    // Sanity check: if the major version the same between compression and decompression?
    if( meta[0] != uint8_t(SPECK_VERSION_MAJOR) )
        return RTNType::VersionMismatch;
    
    // Sanity check: if ZSTD is used consistantly between compression and decompression?
    bool meta_bools[8];
    speck::unpack_8_booleans( meta_bools, meta[1] );

#ifdef USE_ZSTD
    if( meta_bools[0] == false )
        return RTNType::ZSTDMismatch;
#else
    if( meta_bools[0] == true )
        return RTNType::ZSTDMismatch;
#endif

#ifdef USE_ZSTD
    const auto content_size = ZSTD_getFrameContentSize( comp_buf_ptr + meta_size,
                                                        comp_size - meta_size );
    if( content_size == ZSTD_CONTENTSIZE_ERROR || content_size == ZSTD_CONTENTSIZE_UNKNOWN )
        return RTNType::ZSTDError;

    auto content_buf = speck::unique_malloc<uint8_t>(content_size);

    const size_t decomp_size = ZSTD_decompress( content_buf.get(), content_size, 
                               comp_buf_ptr + meta_size, comp_size - meta_size);
    if( ZSTD_isError( decomp_size ) || decomp_size != content_size )
        return RTNType::ZSTDError;

    // Copy over the header
    std::memcpy(header, content_buf.get(), header_size);

    // Now interpret the booleans
    m_bit_buffer.resize( 8 * (content_size - header_size) );
    auto rtn = unpack_booleans( m_bit_buffer, content_buf.get(), content_size, header_size );
    return rtn;
#else
    std::memcpy(header, comp_buf_ptr + meta_size, header_size);
    m_bit_buffer.resize( 8 * (comp_size - header_size - meta_size) );
    auto rtn = unpack_booleans( m_bit_buffer, comp_buf, comp_size, meta_size + header_size );
    return rtn;
#endif
}
#endif


