#include "speck_helper.h"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>



//
// Class SPECKSet1D
//
speck::SPECKSet1D::SPECKSet1D( size_t s, size_t l, uint32_t p )
                 : start(s), length(l), part_level(p)
{ }


//
// Struct Outlier
//
speck::Outlier::Outlier( size_t loc, float e )
              : location(loc), error(e)
{ }

auto speck::num_of_xforms(size_t len) -> size_t
{
    assert(len > 0);
    // I decide 8 is the minimal length to do one level of xform.
    float  f             = std::log2(float(len) / 8.0f);
    size_t num_of_xforms = f < 0.0f ? 0 : size_t(f) + 1;

    return num_of_xforms;
}

auto speck::num_of_partitions(size_t len) -> size_t
{
    size_t num_of_parts = 0; // Num. of partitions we can do 
    while (len > 1) {
        num_of_parts++;
        len -= len / 2;
    }

    return num_of_parts;
}

void speck::calc_approx_detail_len(size_t orig_len, size_t lev,
                                   std::array<size_t, 2>& approx_detail_len)
{
    size_t low_len  = orig_len;
    size_t high_len = 0;
    size_t new_low;
    for (size_t i = 0; i < lev; i++) {
        new_low  = low_len % 2 == 0 ? low_len / 2 : (low_len + 1) / 2;
        high_len = low_len - new_low;
        low_len  = new_low;
    }

    approx_detail_len[0] = low_len;
    approx_detail_len[1] = high_len;
}

template <typename U>
auto speck::make_coeff_positive(U& buf, size_t len, std::vector<bool>& sign_array) 
     -> typename U::element_type
{
    sign_array.assign(len, true);
    auto max                = std::abs(buf[0]);
    using element_type      = typename U::element_type;
    const element_type zero = 0.0;
    for (size_t i = 0; i < len; i++) {
        if (buf[i] < zero) {
            buf[i]        = -buf[i];
            sign_array[i] = false;
        }
        if (buf[i] > max)
            max = buf[i];
    }

    return max;
}
template speck::buffer_type_d::element_type
speck::make_coeff_positive(buffer_type_d&, size_t, std::vector<bool>&);
template speck::buffer_type_f::element_type
speck::make_coeff_positive(buffer_type_f&, size_t, std::vector<bool>&);


// Good solution to deal with bools and unsigned chars
// https://stackoverflow.com/questions/8461126/how-to-create-a-byte-out-of-8-bool-values-and-vice-versa
auto speck::pack_booleans( buffer_type_raw&         dest,
                           const std::vector<bool>& src,
                           size_t                   offset ) -> RTNType
{
    if( src.size() % 8 != 0 )
        return RTNType::WrongSize;

    const uint64_t magic = 0x8040201008040201;
    size_t         pos   = offset;
    bool           a[8];
    uint64_t       t;
    for (size_t i = 0; i < src.size(); i++) {
        auto m = i % 8;
        a[m]   = src[i];
        if (m == 7) // Need to pack 8 booleans!
        {
            std::memcpy(&t, a, 8);
            dest[pos++] = (magic * t) >> 56;
        }
    }

    return RTNType::Good;
}

auto speck::unpack_booleans( std::vector<bool>&    dest,
                             const void*           src,
                             size_t                src_len,
                             size_t                char_offset ) -> RTNType
{
    if( src_len < char_offset )
        return RTNType::WrongSize;

    const uint8_t* src_ptr = static_cast<const uint8_t*>(src);

    size_t num_of_bools = (src_len - char_offset) * 8;
    if( num_of_bools != dest.size() )
        return RTNType::WrongSize;

    const uint64_t magic = 0x8040201008040201;
    const uint64_t mask  = 0x8080808080808080;
    size_t         pos   = char_offset;
    bool           a[8];
    uint64_t       t;
    uint8_t        b;
    for (size_t i = 0; i < num_of_bools; i += 8) {
        std::memcpy( &b, src_ptr + pos++, 1 );
        t = ((magic * b) & mask) >> 7;
        std::memcpy(a, &t, 8);
        for (size_t j = 0; j < 8; j++)
            dest[i + j] = a[j];
    }

    return RTNType::Good;
}

void speck::pack_8_booleans( uint8_t& dest, const bool* src )
{
    const uint64_t magic = 0x8040201008040201;
    uint64_t       t;
    std::memcpy(&t, src, 8);
    dest = (magic * t) >> 56;
}

void speck::unpack_8_booleans( bool* dest, uint8_t src )
{
    const uint64_t magic = 0x8040201008040201;
    const uint64_t mask  = 0x8080808080808080;
    uint64_t t = ((magic * src) & mask) >> 7;
    std::memcpy( dest, &t, 8 );
}

template< typename T >
auto speck::unique_malloc( size_t size ) -> std::unique_ptr<T[]>
{
    std::unique_ptr<T[]> buf = std::make_unique<T[]>(size);
    return std::move( buf );
}
template auto speck::unique_malloc( size_t ) -> buffer_type_c;
template auto speck::unique_malloc( size_t ) -> buffer_type_d;
template auto speck::unique_malloc( size_t ) -> buffer_type_f;
template auto speck::unique_malloc( size_t ) -> buffer_type_raw;
