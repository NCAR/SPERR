#include "SPECK_Storage.h"

#include <cassert>
#include <cstring>
#include <cstdio>

#ifdef USE_ZSTD
    #include "zstd.h"
#endif


template <typename T>
void speck::SPECK_Storage::copy_coeffs(const T* p, size_t len)
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
template void speck::SPECK_Storage::copy_coeffs(const double*, size_t);
template void speck::SPECK_Storage::copy_coeffs(const float*, size_t);

void speck::SPECK_Storage::take_coeffs(buffer_type_d coeffs, size_t len)
{
    assert(len > 0);
    assert(m_coeff_len == 0 || m_coeff_len == len);
    m_coeff_len = len;
    m_coeff_buf = std::move(coeffs);
}

void speck::SPECK_Storage::take_coeffs(buffer_type_f coeffs, size_t len)
{
    // Cannot really take the coeffs if the data type doesn't match.
    // So we make a copy and destroy the old memory block.
    copy_coeffs(coeffs.get(), len);
    coeffs.reset();
}

auto speck::SPECK_Storage::get_read_only_coeffs() const -> const speck::buffer_type_d&
{
    return m_coeff_buf;
}

auto speck::SPECK_Storage::release_coeffs_double() -> speck::buffer_type_d
{
    m_coeff_len = 0;
    return std::move(m_coeff_buf);
}

auto speck::SPECK_Storage::release_coeffs_float() -> speck::buffer_type_f
{
    assert(m_coeff_len > 0);

    buffer_type_f tmp = speck::unique_malloc<float>( m_coeff_len );

    for (size_t i = 0; i < m_coeff_len; i++)
        tmp[i] = m_coeff_buf[i];
    m_coeff_buf = nullptr; // also destroy the current buffer
    return std::move(tmp);
}

void speck::SPECK_Storage::set_image_mean(double mean)
{
    m_image_mean = mean;
}
auto speck::SPECK_Storage::get_image_mean() const -> double
{
    return m_image_mean;
}

auto speck::SPECK_Storage::m_assemble_compressed_buffer( const void* header, 
                                                         size_t header_size,
                                                         buffer_type_raw& out_buf, 
                                                         size_t& out_size) const -> int
{
    // Sanity check on the size of bit_buffer
    if(m_bit_buffer.size() % 8 != 0)
        return 1;

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
    local_buf[0] = (uint8_t)(SPECK_VERSION_MAJOR);
    std::array<bool, 8> meta_bools;
    meta_bools.fill( false );

#ifdef USE_ZSTD
    meta_bools[0] = true;
#endif

    speck::pack_8_booleans( local_buf[1], meta_bools );  // pack the bools to the 2nd byte of buf.

    // Copy over header
    std::memcpy(local_buf.get() + meta_size, header, header_size);

    // Pack booleans to buf!
    speck::pack_booleans( local_buf, m_bit_buffer, meta_size + header_size );

#ifdef USE_ZSTD
    const size_t comp_buf_size = ZSTD_compressBound( header_size + stream_size );

    // We prepend metadata to the new buffer, so allocate space accordingly
    auto comp_buf = speck::unique_malloc<uint8_t>( meta_size + comp_buf_size ); 
    std::memcpy( comp_buf.get(), local_buf.get(), meta_size );

    const size_t comp_size = ZSTD_compress( comp_buf.get() + meta_size, comp_buf_size, 
                             local_buf.get() + meta_size, header_size + stream_size, 
                             ZSTD_CLEVEL_DEFAULT );     // Just use the default compression level.
    if( ZSTD_isError( comp_size ) )
        return 1;

    out_buf  = std::move( comp_buf );
    out_size = meta_size + comp_size;
    
    return 0;
#endif

    out_buf  = std::move( local_buf );
    out_size = total_size;
    return 0;
}


auto speck::SPECK_Storage::m_disassemble_compressed_buffer( void*  header, 
                                                            size_t header_size, 
                                                            const void* comp_buf,
                                                            size_t comp_size) -> int
{
    const size_t meta_size  = 2;    // See m_prepare_compressed_buffer() for the definition 
                                    // of metadata and meta_size
    const uint8_t* comp_buf_ptr = static_cast<const uint8_t*>(comp_buf);

    // Let's parse the metadata
    uint8_t meta[2];
    std::memcpy( meta, comp_buf, sizeof(meta) );

    // Sanity check: if the major version the same between compression and decompression?
    if( meta[0] != (uint8_t)(SPECK_VERSION_MAJOR) )
        return 1;
    
    // Sanity check: if ZSTD is used consistantly between compression and decompression?
    std::array<bool, 8> meta_bools;
    speck::unpack_8_booleans( meta_bools, meta[1] );

#ifdef USE_ZSTD
    if( meta_bools[0] == false )
        return 1;
#else
    if( meta_bools[0] == true )
        return 1;
#endif

#ifdef USE_ZSTD
    const auto content_size = ZSTD_getFrameContentSize( comp_buf_ptr + meta_size,
                                                        comp_size - meta_size );
    if( content_size == ZSTD_CONTENTSIZE_ERROR || content_size == ZSTD_CONTENTSIZE_UNKNOWN )
        return 1;

    auto content_buf = speck::unique_malloc<uint8_t>(content_size);

    const size_t decomp_size = ZSTD_decompress( content_buf.get(), content_size, 
                               comp_buf_ptr + meta_size, comp_size - meta_size);
    if( ZSTD_isError( decomp_size ) || decomp_size != content_size )
        return 1;

    // Copy over the header
    std::memcpy(header, content_buf.get(), header_size);

    // Now interpret the booleans
    m_bit_buffer.resize( 8 * (content_size - header_size) );
    speck::unpack_booleans( m_bit_buffer, content_buf.get(), content_size, header_size );
#else
    std::memcpy(header, comp_buf_ptr + meta_size, header_size);
    m_bit_buffer.resize( 8 * (comp_size - header_size - meta_size) );
    speck::unpack_booleans( m_bit_buffer, comp_buf, comp_size, meta_size + header_size );
#endif

    return 0;
}


auto speck::SPECK_Storage::write_to_disk(const std::string& filename) const -> int
{
    buffer_type_raw out_buf;
    size_t          out_size;
    int rtn = get_compressed_buffer( out_buf, out_size );
    if( rtn != 0 )
        return rtn;

    // Write buffer to a file.
    // It turns out std::fstream isn't as easy to use as c-style file operations, 
    // so let's still use c-style file operations.
    std::FILE* file = std::fopen( filename.c_str(), "wb" );
    if( file ) {

        std::fwrite( out_buf.get(), 1, out_size, file );
        std::fclose( file );
        return 0;
    }
    else {
        return 1;
    }
}


auto speck::SPECK_Storage::read_from_disk(const std::string& filename) -> int
{
    // Open a file and read its content
    // It turns out std::fstream isn't as easy to use as c-style file operations, namely it
    // requires the memory to be of type char*. Let's still use c-style file operations.
    std::FILE* file = std::fopen( filename.c_str(), "rb" );
    if (!file)
        return 1;

    std::fseek( file, 0, SEEK_END );
    const size_t total_size = std::ftell( file );
    std::fseek( file, 0, SEEK_SET );
    const size_t meta_size  = 2;    // See m_assemble_compressed_buffer() for the definition 
                                    // of metadata and meta_size
    auto file_buf = speck::unique_malloc<uint8_t>( total_size );
    size_t nread  = std::fread( file_buf.get(), 1, total_size, file );
    std::fclose( file );
    
    if( total_size != nread )
        return 1;

    return ( read_compressed_buffer( file_buf.get(), total_size ) );
}


auto speck::SPECK_Storage::get_bit_buffer_size() const -> size_t
{
    return m_bit_buffer.size();
}
