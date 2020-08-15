#include "SPECK_Storage.h"

#include <cassert>
#include <cstring>
#include <fstream>

#ifdef USE_ZSTD
    #include "zstd.h"
#endif

template <typename T>
void speck::SPECK_Storage::copy_coeffs(const T& p, size_t len)
{
    assert(len > 0);
    assert(m_coeff_len == 0 || m_coeff_len == len);
    m_coeff_len = len;
#ifdef NO_CPP14
    m_coeff_buf.reset(new double[len]);
#else
    m_coeff_buf       = std::make_unique<double[]>(len);
#endif
    for (size_t i = 0; i < len; i++)
        m_coeff_buf[i] = p[i];
}
template void speck::SPECK_Storage::copy_coeffs(const buffer_type_d&, size_t);
template void speck::SPECK_Storage::copy_coeffs(const buffer_type_f&, size_t);

template <typename T>
void speck::SPECK_Storage::copy_coeffs(const T* p, size_t len)
{
    static_assert(std::is_floating_point<T>::value,
                  "!! Only floating point values are supported !!");

    assert(len > 0);
    assert(m_coeff_len == 0 || m_coeff_len == len);
    m_coeff_len = len;

#ifdef NO_CPP14
    m_coeff_buf.reset(new double[len]);
#else
    m_coeff_buf       = std::make_unique<double[]>(len);
#endif

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
    copy_coeffs(coeffs, len);
    coeffs.reset();
}

void speck::SPECK_Storage::copy_bitstream(const std::vector<bool>& stream)
{
    m_bit_buffer = stream;
}

void speck::SPECK_Storage::take_bitstream(std::vector<bool>& stream)
{
    m_bit_buffer.resize(0);
    std::swap(m_bit_buffer, stream);
}

auto speck::SPECK_Storage::get_read_only_bitstream() const -> const std::vector<bool>&
{
    return m_bit_buffer;
}

auto speck::SPECK_Storage::get_read_only_coeffs() const -> const speck::buffer_type_d&
{
    return m_coeff_buf;
}

auto speck::SPECK_Storage::release_bitstream() -> std::vector<bool>
{
    return std::move( m_bit_buffer );
}

auto speck::SPECK_Storage::release_coeffs_double() -> speck::buffer_type_d
{
    m_coeff_len = 0;
    return std::move(m_coeff_buf);
}

auto speck::SPECK_Storage::release_coeffs_float() -> speck::buffer_type_f
{
    assert(m_coeff_len > 0);

#ifdef NO_CPP14
    buffer_type_f tmp(new float[m_coeff_len]);
#else
    buffer_type_f tmp = std::make_unique<float[]>(m_coeff_len);
#endif

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

// Good solution to deal with bools and unsigned chars
// https://stackoverflow.com/questions/8461126/how-to-create-a-byte-out-of-8-bool-values-and-vice-versa
auto speck::SPECK_Storage::m_write(const void* header, size_t header_size,
                                   const char* filename) const -> int
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

#ifdef NO_CPP14
    buffer_type_c buf(new char[total_size]);
#else
    buffer_type_c buf = std::make_unique<char[]>(total_size);
#endif

    // Fill in metadata as defined above
    buf[0] = char(SPECK_VERSION_MAJOR);
    std::array<bool, 8> meta_bools;
    meta_bools.fill( false );

#ifdef USE_ZSTD
    meta_bools[0] = true;
#endif

    speck::pack_8_booleans( buf.get() + 1, meta_bools );  // pack the bools to the 2nd byte of buf.

    // Copy over header
    std::memcpy(buf.get() + meta_size, header, header_size);

    // Pack booleans to buf!
    speck::pack_booleans( buf, m_bit_buffer, meta_size + header_size );

#ifdef USE_ZSTD
    const size_t comp_buf_size = ZSTD_compressBound( header_size + stream_size );

    // We prepend metadata to the new buffer, so allocate space accordingly

    #ifdef NO_CPP14
        buffer_type_c comp_buf(new char[meta_size + comp_buf_size]);
    #else
        buffer_type_c comp_buf = std::make_unique<char[]>(meta_size + comp_buf_size);
    #endif

    std::memcpy( comp_buf.get(), buf.get(), meta_size );

    const size_t comp_size = ZSTD_compress( comp_buf.get() + meta_size, comp_buf_size, 
                             buf.get() + meta_size, header_size + stream_size, 
                             ZSTD_CLEVEL_DEFAULT );     // Just use the default compression level.
    if( ZSTD_isError( comp_size ) )
        return 1;
#endif

    // Write buffer to a file.
    // Good introduction here: http://www.cplusplus.com/doc/tutorial/files/
    std::ofstream file(filename, std::ios::binary);
    if (file.is_open()) {

    #ifdef USE_ZSTD
        file.write(comp_buf.get(), meta_size + comp_size);
    #else
        file.write(buf.get(), total_size);
    #endif

        file.close();
        return 0;
    } 
    else {
        return 1;
    }
}

auto speck::SPECK_Storage::m_read(void* header, size_t header_size, const char* filename) -> int
{
    // Open a file and read its content
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
        return 1;
    file.seekg(0, file.end);
    const size_t total_size = file.tellg();
    const size_t meta_size  = 2;    // See m_write() for the definition of metadata and meta_size
    file.seekg(0, file.beg);

#ifdef NO_CPP14
    buffer_type_c file_buf(new char[total_size]);
#else
    buffer_type_c file_buf = std::make_unique<char[]>(total_size);
#endif

    file.read(file_buf.get(), total_size);
    file.close();

    // Sanity check: if the major version the same between compression and decompression?
    if( file_buf[0] != char(SPECK_VERSION_MAJOR) )
        return 0;
    
    // Sanity check: if ZSTD is used consistantly between compression and decompression?
    std::array<bool, 8> meta_bools;
    speck::unpack_8_booleans( meta_bools, file_buf.get() + 1 );

#ifdef USE_ZSTD
    if( meta_bools[0] == false )
        return 1;
#else
    if( meta_bools[0] == true )
        return 1;
#endif


#ifdef USE_ZSTD
    const unsigned long long content_size = ZSTD_getFrameContentSize( file_buf.get() + meta_size,
                                                                      total_size - meta_size );
    if( content_size == ZSTD_CONTENTSIZE_ERROR || content_size == ZSTD_CONTENTSIZE_UNKNOWN )
        return 1;

    #ifdef NO_CPP14
        buffer_type_c content_buf(new char[content_size]);
    #else
        buffer_type_c content_buf = std::make_unique<char[]>(content_size);
    #endif

    const size_t decomp_size = ZSTD_decompress( content_buf.get(), content_size, 
                               file_buf.get() + meta_size, total_size - meta_size);
    if( ZSTD_isError( decomp_size ) || decomp_size != content_size )
        return 1;

    // Copy over the header
    std::memcpy(header, content_buf.get(), header_size);
    // Now interpret the booleans
    m_bit_buffer.resize( 8 * (content_size - header_size) );
    speck::unpack_booleans( m_bit_buffer, content_buf, content_size, header_size );
#else
    std::memcpy(header, file_buf.get() + meta_size, header_size);
    m_bit_buffer.resize( 8 * (total_size - header_size - meta_size) );
    speck::unpack_booleans( m_bit_buffer, file_buf, total_size, meta_size + header_size );
#endif

    return 0;
}

auto speck::SPECK_Storage::get_bit_buffer_size() const -> size_t
{
    return m_bit_buffer.size();
}
