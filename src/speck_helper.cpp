#include "speck_helper.h"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>


//
// Class SPECKSet3D
//
auto speck::SPECKSet3D::is_pixel() const -> bool
{
    return (length_x == 1 && length_y == 1 && length_z == 1);
}

auto speck::SPECKSet3D::is_empty() const -> bool
{
    return (length_z == 0 || length_y == 0 || length_x == 0);
}


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

#ifdef PRINT
void speck::SPECKSet3D::print() const
{
    printf("  start: (%d, %d, %d), length: (%d, %d, %d). Part_level: %d\n",
           start_x, start_y, start_z, length_x, length_y, length_z, part_level);
}
#endif

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

void speck::partition_S_XYZ(const SPECKSet3D& set, std::array<SPECKSet3D, 8>& subsets)
{
    const uint32_t split_x[2] { set.length_x - set.length_x / 2, set.length_x / 2 };
    const uint32_t split_y[2] { set.length_y - set.length_y / 2, set.length_y / 2 };
    const uint32_t split_z[2] { set.length_z - set.length_z / 2, set.length_z / 2 };

    for (auto& s : subsets) {
        s.part_level = set.part_level;
        if (split_x[1] > 0)
            s.part_level++;
        if (split_y[1] > 0)
            s.part_level++;
        if (split_z[1] > 0)
            s.part_level++;
    }

    //
    // The actual figuring out where it starts/ends part...
    //
    const size_t offsets[3] { 1, 2, 4 };
    // subset (0, 0, 0)
    size_t sub_i  = 0 * offsets[0] + 0 * offsets[1] + 0 * offsets[2];
    auto&  sub0   = subsets[sub_i];
    sub0.start_x  = set.start_x;
    sub0.length_x = split_x[0];
    sub0.start_y  = set.start_y;
    sub0.length_y = split_y[0];
    sub0.start_z  = set.start_z;
    sub0.length_z = split_z[0];

    // subset (1, 0, 0)
    sub_i         = 1 * offsets[0] + 0 * offsets[1] + 0 * offsets[2];
    auto& sub1    = subsets[sub_i];
    sub1.start_x  = set.start_x + split_x[0];
    sub1.length_x = split_x[1];
    sub1.start_y  = set.start_y;
    sub1.length_y = split_y[0];
    sub1.start_z  = set.start_z;
    sub1.length_z = split_z[0];

    // subset (0, 1, 0)
    sub_i         = 0 * offsets[0] + 1 * offsets[1] + 0 * offsets[2];
    auto& sub2    = subsets[sub_i];
    sub2.start_x  = set.start_x;
    sub2.length_x = split_x[0];
    sub2.start_y  = set.start_y + split_y[0];
    sub2.length_y = split_y[1];
    sub2.start_z  = set.start_z;
    sub2.length_z = split_z[0];

    // subset (1, 1, 0)
    sub_i         = 1 * offsets[0] + 1 * offsets[1] + 0 * offsets[2];
    auto& sub3    = subsets[sub_i];
    sub3.start_x  = set.start_x + split_x[0];
    sub3.length_x = split_x[1];
    sub3.start_y  = set.start_y + split_y[0];
    sub3.length_y = split_y[1];
    sub3.start_z  = set.start_z;
    sub3.length_z = split_z[0];

    // subset (0, 0, 1)
    sub_i         = 0 * offsets[0] + 0 * offsets[1] + 1 * offsets[2];
    auto& sub4    = subsets[sub_i];
    sub4.start_x  = set.start_x;
    sub4.length_x = split_x[0];
    sub4.start_y  = set.start_y;
    sub4.length_y = split_y[0];
    sub4.start_z  = set.start_z + split_z[0];
    sub4.length_z = split_z[1];

    // subset (1, 0, 1)
    sub_i         = 1 * offsets[0] + 0 * offsets[1] + 1 * offsets[2];
    auto& sub5    = subsets[sub_i];
    sub5.start_x  = set.start_x + split_x[0];
    sub5.length_x = split_x[1];
    sub5.start_y  = set.start_y;
    sub5.length_y = split_y[0];
    sub5.start_z  = set.start_z + split_z[0];
    sub5.length_z = split_z[1];

    // subset (0, 1, 1)
    sub_i         = 0 * offsets[0] + 1 * offsets[1] + 1 * offsets[2];
    auto& sub6    = subsets[sub_i];
    sub6.start_x  = set.start_x;
    sub6.length_x = split_x[0];
    sub6.start_y  = set.start_y + split_y[0];
    sub6.length_y = split_y[1];
    sub6.start_z  = set.start_z + split_z[0];
    sub6.length_z = split_z[1];

    // subset (1, 1, 1)
    sub_i         = 1 * offsets[0] + 1 * offsets[1] + 1 * offsets[2];
    auto& sub7    = subsets[sub_i];
    sub7.start_x  = set.start_x + split_x[0];
    sub7.length_x = split_x[1];
    sub7.start_y  = set.start_y + split_y[0];
    sub7.length_y = split_y[1];
    sub7.start_z  = set.start_z + split_z[0];
    sub7.length_z = split_z[1];
}

void speck::partition_S_XY(const SPECKSet3D& set, std::array<SPECKSet3D, 4>& subsets)
{
    const uint32_t split_x[2] { set.length_x - set.length_x / 2, set.length_x / 2 };
    const uint32_t split_y[2] { set.length_y - set.length_y / 2, set.length_y / 2 };

    for (auto& s : subsets) {
        s.part_level = set.part_level;
        if (split_x[1] > 0)
            s.part_level++;
        if (split_y[1] > 0)
            s.part_level++;
    }

    //
    // The actual figuring out where it starts/ends part...
    //
    const size_t offsets[3] { 1, 2, 4 };
    // subset (0, 0, 0)
    size_t sub_i  = 0 * offsets[0] + 0 * offsets[1] + 0 * offsets[2];
    auto&  sub0   = subsets[sub_i];
    sub0.start_x  = set.start_x;
    sub0.length_x = split_x[0];
    sub0.start_y  = set.start_y;
    sub0.length_y = split_y[0];
    sub0.start_z  = set.start_z;
    sub0.length_z = set.length_z;

    // subset (1, 0, 0)
    sub_i         = 1 * offsets[0] + 0 * offsets[1] + 0 * offsets[2];
    auto& sub1    = subsets[sub_i];
    sub1.start_x  = set.start_x + split_x[0];
    sub1.length_x = split_x[1];
    sub1.start_y  = set.start_y;
    sub1.length_y = split_y[0];
    sub1.start_z  = set.start_z;
    sub1.length_z = set.length_z;

    // subset (0, 1, 0)
    sub_i         = 0 * offsets[0] + 1 * offsets[1] + 0 * offsets[2];
    auto& sub2    = subsets[sub_i];
    sub2.start_x  = set.start_x;
    sub2.length_x = split_x[0];
    sub2.start_y  = set.start_y + split_y[0];
    sub2.length_y = split_y[1];
    sub2.start_z  = set.start_z;
    sub2.length_z = set.length_z;

    // subset (1, 1, 0)
    sub_i         = 1 * offsets[0] + 1 * offsets[1] + 0 * offsets[2];
    auto& sub3    = subsets[sub_i];
    sub3.start_x  = set.start_x + split_x[0];
    sub3.length_x = split_x[1];
    sub3.start_y  = set.start_y + split_y[0];
    sub3.length_y = split_y[1];
    sub3.start_z  = set.start_z;
    sub3.length_z = set.length_z;
}

void speck::partition_S_Z(const SPECKSet3D& set, std::array<SPECKSet3D, 2>& subsets)
{
    const uint32_t split_z[2] { set.length_z - set.length_z / 2, set.length_z / 2 };

    for (auto& s : subsets) {
        s.part_level = set.part_level;
        if (split_z[1] > 0)
            s.part_level++;
    }

    //
    // The actual figuring out where it starts/ends part...
    //
    // subset (0, 0, 0)
    auto& sub0    = subsets[0];
    sub0.start_x  = set.start_x;
    sub0.length_x = set.length_x;
    sub0.start_y  = set.start_y;
    sub0.length_y = set.length_y;
    sub0.start_z  = set.start_z;
    sub0.length_z = split_z[0];

    // subset (0, 0, 1)
    auto& sub1    = subsets[1];
    sub1.start_x  = set.start_x;
    sub1.length_x = set.length_x;
    sub1.start_y  = set.start_y;
    sub1.length_y = set.length_y;
    sub1.start_z  = set.start_z + split_z[0];
    sub1.length_z = split_z[1];
}

auto speck::pack_booleans( buffer_type_c&           dest,
                           const std::vector<bool>& src,
                           size_t                   offset ) -> int
{
    if( src.size() % 8 != 0 )
        return 1;

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

    return 0;
}

auto speck::unpack_booleans( std::vector<bool>&    dest,
                             const buffer_type_c&  src,
                             size_t                src_len,
                             size_t                char_offset ) -> int
{
    if( src_len < char_offset )
        return 1;

    size_t num_of_bools = (src_len - char_offset) * 8;
    if( num_of_bools != dest.size() )
        return 1;

    const uint64_t magic = 0x8040201008040201;
    const uint64_t mask  = 0x8080808080808080;
    size_t         pos   = char_offset;
    bool           a[8];
    uint64_t       t;
    uint8_t        b;
    for (size_t i = 0; i < num_of_bools; i += 8) {
        b = src[pos++];
        t = ((magic * b) & mask) >> 7;
        std::memcpy(a, &t, 8);
        for (size_t j = 0; j < 8; j++)
            dest[i + j] = a[j];
    }

    return 0;
}

void speck::pack_8_booleans( uint8_t& dest, const std::array<bool, 8>& src )
{
    const uint64_t magic = 0x8040201008040201;
    uint64_t       t;
    std::memcpy(&t, src.data(), 8);
    dest = (magic * t) >> 56;
}

void speck::unpack_8_booleans( std::array<bool, 8>& dest, uint8_t src )
{
    const uint64_t magic = 0x8040201008040201;
    const uint64_t mask  = 0x8080808080808080;
    uint64_t t = ((magic * src) & mask) >> 7;
    std::memcpy( dest.data(), &t, 8 );
}

