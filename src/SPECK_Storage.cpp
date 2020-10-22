#include "SPECK_Storage.h"

#include <cassert>
#include <cstring>
#include <cstdio>

#ifdef USE_ZSTD
    #include "zstd.h"
#endif


#ifdef USE_PMR
speck::SPECK_Storage::SPECK_Storage()
                    : m_bit_buffer( std::pmr::polymorphic_allocator<bool>(&m_pool) )
{
    // Set the default resource, so PMR objects created subsequently in this class
    //   (and its subclasses) will use m_pool as the default resource.
    std::pmr::set_default_resource( &m_pool );

    // Note that m_bit_buffer was created before this default is set,
    //   it was constructed with an explicit allocator.
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
    m_coeff_buf = unique_malloc<double>(len);
    for (size_t i = 0; i < len; i++)
        m_coeff_buf[i] = p[i];
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

auto speck::SPECK_Storage::m_assemble_encoded_bitstream( const void* header, 
                                                         size_t header_size ) const
                           -> std::pair<buffer_type_raw, size_t>
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


auto speck::SPECK_Storage::get_bit_buffer_size() const -> size_t
{
    return m_bit_buffer.size();
}
