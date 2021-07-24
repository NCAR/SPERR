#include "speck_helper.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

//
// Uncomment the following lines to enable OpenMP
//
// #include <omp.h>


auto speck::num_of_xforms(size_t len) -> size_t
{
    assert(len > 0);
    // I decide 8 is the minimal length to do one level of xform.
    float  f = std::log2(float(len) / 8.0f);
    return ( f < 0.0f ? 0 : size_t(f) + 1 );
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

auto speck::calc_approx_detail_len(size_t orig_len, size_t lev) -> std::array<size_t, 2>
{
    size_t low_len  = orig_len;
    size_t high_len = 0;
    for (size_t i = 0; i < lev; i++) {
        high_len = low_len / 2;
        low_len -= high_len;
    }

    return {low_len, high_len};
}

auto speck::make_coeff_positive(vecd_type& buf, std::vector<bool>& signs) -> double
{
    signs.resize( buf.size(), false );
    auto max = std::abs(buf[0]);

    const auto tmpb = std::array<bool, 2> {true, false};

    for (size_t i = 0; i < buf.size(); i++) {
        auto tmpd = std::array<double, 2> {buf[i], -buf[i]};
        size_t idx = buf[i] < 0.0;
        buf[i]   = tmpd[idx];
        signs[i] = tmpb[idx];
        max      = std::max(max, buf[i]);
    }

    return max;
}


// Good solution to deal with bools and unsigned chars
// https://stackoverflow.com/questions/8461126/how-to-create-a-byte-out-of-8-bool-values-and-vice-versa
auto speck::pack_booleans( std::vector<uint8_t>&    dest,
                           const std::vector<bool>& src,
                           size_t                   offset ) -> RTNType
{
    if( src.size() % 8 != 0 ) // `src` has to have a size of multiples of 8.
        return RTNType::WrongSize;

    const uint64_t magic = 0x8040201008040201;

    // It turns out C++ doesn't specify bool to be 1 byte, so we have to use
    // uint8_t here which is definitely 1 byte in size.
    // Also, C++ guarantees conversion between booleans and integers:
    // true <--> 1, false <--> 0.
    uint8_t  a[8]{0}; // NOLINT
    uint64_t t = 0;
    size_t   dest_idx = offset;
    for( size_t i = 0; i < src.size(); i += 8 ) {
        #pragma GCC unroll 8
        for( size_t j = 0; j < 8; j++ )
            a[j] = src[i + j];
        std::memcpy( &t, a, 8 );
        dest[ dest_idx++ ] = (magic * t) >> 56;
    }

    return RTNType::Good;
}

auto speck::unpack_booleans( std::vector<bool>& dest,
                             const void*        src,
                             size_t             src_len,
                             size_t             src_offset ) -> RTNType
{
    if( src == nullptr )
        return RTNType::InvalidParam;

    if( src_len < src_offset )
        return RTNType::WrongSize;

    const size_t num_of_bytes = src_len - src_offset;
    const size_t num_of_bools = num_of_bytes * 8;
    if( dest.size() != num_of_bools )
        dest.resize( num_of_bools );

    const uint8_t* src_ptr = reinterpret_cast<const uint8_t*>(src) + src_offset;
    const uint64_t magic   = 0x8040201008040201;
    const uint64_t mask    = 0x8080808080808080;

#ifndef OMP_UNPACK_BOOLEANS
    uint8_t  a[8]{0}; // NOLINT
    uint64_t t = 0;
    size_t   dest_idx = 0;
    for( size_t byte_idx = 0; byte_idx < num_of_bytes; byte_idx++ ) {
        const uint8_t* ptr = src_ptr + byte_idx;
        t = (( magic * (*ptr) ) & mask) >> 7;
        std::memcpy( a, &t, 8 );
        #pragma GCC unroll 8
        for( size_t i = 0; i < 8; i++ )
            dest[ dest_idx + i ] = a[i];
        dest_idx += 8;
    }
#else
    // Because in most implementations std::vector<bool> is stored as uint64_t values,
    //   we parallel in strides of 64 bits, or 8 bytes.
    const size_t stride_size    = 8;
    const size_t num_of_strides = num_of_bytes / stride_size;

    #pragma omp parallel for
    for( size_t stride = 0; stride < num_of_strides; stride++ ) {
        uint8_t a[64]{0};
        for( size_t byte = 0; byte < stride_size; byte++ ) {
            const uint8_t* ptr = src_ptr + stride * stride_size + byte;
            const uint64_t t = (( magic * (*ptr) ) & mask) >> 7;
            std::memcpy( a + byte * 8, &t, 8 );
        }
        for( size_t i = 0; i < 64; i++ )
            dest[ stride * 64 + i ] = a[i];
    }
    // This loop is at most 7 iterations, so not to worry about parallel anymore.
    for( size_t byte_idx = stride_size * num_of_strides; byte_idx < num_of_bytes; byte_idx++ ) {
        const uint8_t* ptr = src_ptr + byte_idx;
        const uint64_t t   = (( magic * (*ptr) ) & mask) >> 7;
        uint8_t a[8]{0};
        std::memcpy( a, &t, 8 );
        for( size_t i = 0; i < 8; i++ )
            dest[ byte_idx * 8 + i ] = a[i];
    }
#endif

    return RTNType::Good;
}

void speck::pack_8_booleans( uint8_t& dest, const bool* src )
{
    const uint64_t  magic = 0x8040201008040201;
    uint64_t t = 0;
    uint8_t  a[8]{0}; // NOLINT
    for( size_t i = 0; i < 8; i++ )
        a[i] = src[i];
    std::memcpy(&t, a, 8);
    dest = (magic * t) >> 56;
}

void speck::unpack_8_booleans( bool* dest, uint8_t src )
{
    const uint64_t magic = 0x8040201008040201;
    const uint64_t mask  = 0x8080808080808080;
    uint64_t t = ((magic * src) & mask) >> 7;
    uint8_t  a[8]{0}; // NOLINT
    std::memcpy( a, &t, 8 );
    for( size_t i = 0; i < 8; i++ )
        dest[i] = a[i];
}


auto speck::read_n_bytes( const char* filename, size_t n_bytes, void* buffer ) -> RTNType
{
    std::unique_ptr<std::FILE, decltype(&std::fclose)> fp (std::fopen(filename, "rb"), &std::fclose);

    if( !fp )
        return RTNType::IOError;

    std::fseek( fp.get(), 0, SEEK_END );
    if( std::ftell(fp.get()) < n_bytes )
        return RTNType::InvalidParam;

    std::fseek( fp.get(), 0, SEEK_SET );
    if( std::fread( buffer, 1, n_bytes, fp.get() ) != n_bytes )
        return RTNType::IOError;

    return RTNType::Good;
}


template <typename T>
auto speck::read_whole_file( const char* filename ) -> std::vector<T>
{
    std::vector<T> buf;

    std::unique_ptr<std::FILE, decltype(&std::fclose)> fp (std::fopen(filename, "rb"), &std::fclose);
    if( !fp )
        return buf;

    std::fseek( fp.get(), 0, SEEK_END );
    const size_t file_size = std::ftell( fp.get() );
    const size_t num_vals  = file_size / sizeof(T);
    std::fseek( fp.get(), 0, SEEK_SET );

    buf.resize( num_vals );
    size_t nread  = std::fread( buf.data(), sizeof(T), num_vals, fp.get() );
    if( nread != num_vals )
        buf.clear();

    return buf;
}
template auto speck::read_whole_file( const char* ) -> std::vector<float>;
template auto speck::read_whole_file( const char* ) -> std::vector<double>;
template auto speck::read_whole_file( const char* ) -> std::vector<uint8_t>;


auto speck::write_n_bytes( const char* filename, size_t n_bytes, const void* buffer ) -> RTNType
{
    std::unique_ptr<std::FILE, decltype(&std::fclose)> fp (std::fopen(filename, "wb"), &std::fclose);
    if( !fp )
        return RTNType::IOError;

    if( std::fwrite(buffer, 1, n_bytes, fp.get()) != n_bytes )
        return RTNType::IOError;
    else
        return RTNType::Good;
}


template <typename T>
void speck::calc_stats( const T* arr1,   const T* arr2,  size_t len,
                        T* rmse, T* linfty, T* psnr, T* arr1min, T* arr1max )
{
    const size_t stride_size    = 4096;
    const size_t num_of_strides = len / stride_size;
    const size_t remainder_size = len - stride_size * num_of_strides;

    auto sum_vec    = std::vector<T>( num_of_strides + 1);
    auto linfty_vec = std::vector<T>( num_of_strides + 1);

    //
    // Calculate summation and l-infty of each stride
    //
    // (Uncomment the following line to enable OpenMP)
    //
    // #pragma omp parallel for
    for( size_t stride_i = 0; stride_i < num_of_strides; stride_i++ ) {
        T linfty = 0.0;
        auto buf = std::array<T, stride_size>();
        for( size_t i = 0; i < stride_size; i++ ) {
            const size_t idx = stride_i * stride_size + i;
            auto diff = std::abs( arr1[idx] - arr2[idx] );
            linfty    = std::max( linfty, diff );
            buf[i]    = diff * diff;
        }
        sum_vec   [ stride_i ] = speck::kahan_summation( buf.data(), stride_size );
        linfty_vec[ stride_i ] = linfty;
    }

    //
    // Calculate summation and l-infty of the remaining elements
    //
    T last_linfty = 0.0;
    auto last_buf = std::array<T, stride_size>{}; // must be enough for `remainder_size` elements.
    for( size_t i = 0; i < remainder_size; i++ ) {
        const size_t idx = stride_size * num_of_strides + i;
        auto diff   = std::abs( arr1[idx] - arr2[idx] );
        last_linfty = std::max( last_linfty, diff );
        last_buf[i] = diff * diff;
    }
    sum_vec   [ num_of_strides ] = speck::kahan_summation( last_buf.data(), remainder_size );
    linfty_vec[ num_of_strides ] = last_linfty;

    //
    // Now calculate min, max, linfty
    //
    const auto minmax = std::minmax_element( arr1, arr1 + len );
    *arr1min = *minmax.first;
    *arr1max = *minmax.second;
    *linfty  = *(std::max_element(linfty_vec.begin(), linfty_vec.end()));

    //
    // Now calculate rmse and psnr
    // Note: psnr is calculated in dB, and follows the equation described in:
    // http://homepages.inf.ed.ac.uk/rbf/CVonline/LOCAL_COPIES/VELDHUIZEN/node18.html
    // Also refer to https://www.mathworks.com/help/vision/ref/psnr.html
    //
    const auto msr = speck::kahan_summation( sum_vec.data(), sum_vec.size() ) / T(len);
    *rmse = std::sqrt( msr );
    auto range_sq = *minmax.first - *minmax.second;
    range_sq *= range_sq;
    *psnr = std::log10( range_sq / msr ) * 10.0;
}
template void speck::calc_stats( const float*, const float*, size_t, 
                                 float*, float*, float*, float*, float* );
template void speck::calc_stats( const double*, const double*, size_t, 
                                 double*, double*, double*, double*, double* );


template <typename T>
auto speck::kahan_summation( const T* arr, size_t len ) -> T
{
    T sum = 0.0, c = 0.0;
    T t, y;
    for( size_t i = 0; i < len; i++) {
        y   = arr[i] - c;
        t   = sum + y;
        c   = (t - sum) - y;
        sum = t;
    }

    return sum;
}
template auto speck::kahan_summation( const float*,  size_t ) -> float;
template auto speck::kahan_summation( const double*, size_t ) -> double;


auto speck::chunk_volume( const std::array<size_t, 3>& vol_dim, 
                          const std::array<size_t, 3>& chunk_dim )
                          -> std::vector< std::array<size_t, 6> >
{
    // Step 1: figure out how many segments are there along each axis.
    auto n_segs = std::array<size_t, 3>();
    for( size_t i = 0; i < 3; i++ ) {
        n_segs[i] = vol_dim[i] / chunk_dim[i];
        if( (vol_dim[i] % chunk_dim[i]) > (chunk_dim[i] / 2) )
            n_segs[i]++;
        // In case the volume has one dimension that's too small, let's make it
        // at least one segment anyway.
        if( n_segs[i] == 0 )
            n_segs[i] =  1;
    }

    // Step 2: calculate the starting indices of each segment along each axis.
    auto x_tics = std::vector<size_t>( n_segs[0] + 1 );
    for( size_t i = 0; i < n_segs[0]; i++ )
        x_tics[i] = i * chunk_dim[0];
    x_tics[n_segs[0]] = vol_dim[0]; // the last tic is the length in X

    auto y_tics = std::vector<size_t>( n_segs[1] + 1 );
    for( size_t i = 0; i < n_segs[1]; i++ )
        y_tics[i] = i * chunk_dim[1];
    y_tics[n_segs[1]] = vol_dim[1]; // the last tic is the length in Y

    auto z_tics = std::vector<size_t>( n_segs[2] + 1 );
    for( size_t i = 0; i < n_segs[2]; i++ )
        z_tics[i] = i * chunk_dim[2];
    z_tics[n_segs[2]] = vol_dim[2]; // the last tic is the length in Z

    // Step 3: fill in details of each chunk
    auto n_chunks = n_segs[0] * n_segs[1] * n_segs[2];
    auto chunks   = std::vector< std::array<size_t, 6> >( n_chunks );
    size_t chunk_idx = 0;
    for( size_t z = 0; z < n_segs[2]; z++ )
      for( size_t y = 0; y < n_segs[1]; y++ )
        for( size_t x = 0; x < n_segs[0]; x++ ) {
            chunks[chunk_idx][0] = x_tics[x];                   // X start
            chunks[chunk_idx][1] = x_tics[x + 1] - x_tics[x];   // X length
            chunks[chunk_idx][2] = y_tics[y];                   // Y start
            chunks[chunk_idx][3] = y_tics[y + 1] - y_tics[y];   // Y length
            chunks[chunk_idx][4] = z_tics[z];                   // Z start
            chunks[chunk_idx][5] = z_tics[z + 1] - z_tics[z];   // Z length
            chunk_idx++;
        }

    return std::move(chunks);
}


template<typename T>
auto speck::gather_chunk( const T* vol,     dims_type vol_dim,
                          const std::array<size_t, 6>& chunk ) -> vecd_type
{
    auto buf = std::vector<double>();
    if( chunk[0] + chunk[1] > vol_dim[0] || 
        chunk[2] + chunk[3] > vol_dim[1] ||
        chunk[4] + chunk[5] > vol_dim[2] )
        return buf;

    auto len = chunk[1] * chunk[3] * chunk[5];
    buf.resize(len);

    size_t idx = 0;
    for( size_t z = chunk[4]; z < chunk[4] + chunk[5]; z++ ) {
      const size_t plane_offset = z * vol_dim[0] * vol_dim[1];
      for( size_t y = chunk[2]; y < chunk[2] + chunk[3]; y++ ) {
        const size_t col_offset = plane_offset + y * vol_dim[0];
        for( size_t x = chunk[0]; x < chunk[0] + chunk[1]; x++ )
          buf[idx++] = vol[ col_offset + x ];
      }
    }

    // Will be subject to Named Return Value Optimization.
    return buf;
}
template auto speck::gather_chunk( const float*,    dims_type,
                                   const std::array<size_t, 6>& ) -> vecd_type;
template auto speck::gather_chunk( const double*,   dims_type,
                                   const std::array<size_t, 6>& ) -> vecd_type;


void speck::scatter_chunk( vecd_type& big_vol,  dims_type vol_dim,
                           const vecd_type&     small_vol,
                           const std::array<size_t, 6>& chunk )
{
    size_t idx = 0;
    for( size_t z = chunk[4]; z < chunk[4] + chunk[5]; z++ ) {
      const size_t plane_offset = z * vol_dim[0] * vol_dim[1];
      for( size_t y = chunk[2]; y < chunk[2] + chunk[3]; y++ ) {
        const size_t col_offset = plane_offset + y * vol_dim[0];
        for( size_t x = chunk[0]; x < chunk[0] + chunk[1]; x++ )
          big_vol[ col_offset + x ] = small_vol[ idx++ ];
      }
    }
}





