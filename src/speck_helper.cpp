#include "speck_helper.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <algorithm>

//
// Uncomment the following lines to enable OpenMP
//
// #include <omp.h>


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

    // Notice that the use of std::vector<bool> for sign_array prevents 
    //   this loop been parallelized using OpenMP.
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
auto speck::pack_booleans( buffer_type_uint8&       dest,
                           const std::vector<bool>& src,
                           size_t                   offset ) -> RTNType
{
    if( src.size() % 8 != 0 )
        return RTNType::WrongSize;

    const uint64_t magic = 0x8040201008040201;

    //
    // Uncomment the following line to enable OpenMP
    //
    // #pragma omp parallel for
    for( size_t i = 0; i < src.size(); i += 8 ) {
        bool a[8];
        for( size_t j = 0; j < 8; j++ )
            a[j] = src[i + j];
        uint64_t t;
        std::memcpy( &t, a, 8 );
        dest[ offset + i / 8 ] = (magic * t) >> 56;
    }

    return RTNType::Good;
}

auto speck::unpack_booleans( std::vector<bool>& dest,
                             const void*        src,
                             size_t             src_len,
                             size_t             src_offset ) -> RTNType
{
    if( src_len < src_offset )
        return RTNType::WrongSize;

    const size_t num_of_bytes = src_len - src_offset;
    const size_t num_of_bools = num_of_bytes * 8;
    if( dest.size() != num_of_bools )
        dest.resize( num_of_bools );

    const uint8_t* src_ptr = reinterpret_cast<const uint8_t*>(src) + src_offset;
    const uint64_t magic   = 0x8040201008040201;
    const uint64_t mask    = 0x8080808080808080;

    // Because in most implementations std::vector<bool> is stored as uint64_t values,
    //   we parallel in strides of 64 bits, or 8 bytes..
    const size_t stride_size = 8;
    const size_t num_of_strides  = num_of_bytes / stride_size;

    //
    // Uncomment the following line to enable OpenMP
    //
    // #pragma omp parallel for
    for( size_t stride = 0; stride < num_of_strides; stride++ ) {
        bool a[64];
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
        const uint64_t t = (( magic * (*ptr) ) & mask) >> 7;
        bool  a[8];
        std::memcpy( a, &t, 8 );
        for( size_t i = 0; i < 8; i++ )
            dest[ byte_idx * 8 + i ] = a[i];
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


template<typename T>
auto speck::begin( const std::unique_ptr<T[]>& uptr ) -> ptr_iterator<T>
{
    return ptr_iterator<T>( uptr.get() );
}
template<typename T>
auto speck::end( const std::unique_ptr<T[]>& uptr, size_t length ) -> ptr_iterator<T>
{
    return ptr_iterator<T>( uptr.get() + length );
}
template auto speck::begin(const speck::buffer_type_f& )         -> ptr_iterator<float >;
template auto speck::begin(const speck::buffer_type_d& )         -> ptr_iterator<double>;
template auto speck::end(  const speck::buffer_type_f&, size_t ) -> ptr_iterator<float >;
template auto speck::end(  const speck::buffer_type_d&, size_t ) -> ptr_iterator<double>;


template<typename T>
auto speck::begin( const std::pair<std::unique_ptr<T[]>, size_t>& sbf ) -> ptr_iterator<T>
{
    return ptr_iterator<T>( sbf.first.get() );
}
template<typename T>
auto speck::end( const std::pair<std::unique_ptr<T[]>, size_t>& sbf ) -> ptr_iterator<T>
{
    return ptr_iterator<T>( sbf.first.get() + sbf.second );
}
template auto speck::begin( const speck::smart_buffer_f& ) -> ptr_iterator<float >;
template auto speck::begin( const speck::smart_buffer_d& ) -> ptr_iterator<double>;
template auto speck::end(   const speck::smart_buffer_f& ) -> ptr_iterator<float >;
template auto speck::end(   const speck::smart_buffer_d& ) -> ptr_iterator<double>;


auto speck::read_n_bytes( const char* filename, size_t n_bytes, void* buffer ) -> RTNType
{
    std::FILE* f = std::fopen( filename, "rb" );
    if( !f ) {
        return RTNType::IOError;
    }
    std::fseek( f, 0, SEEK_END );
    if( std::ftell(f) < n_bytes ) {
        std::fclose( f );
        return RTNType::InvalidParam;
    }
    std::fseek( f, 0, SEEK_SET );
    if( std::fread( buffer, 1, n_bytes, f ) != n_bytes ) {
        std::fclose( f );
        return RTNType::IOError;
    }
    std::fclose( f );
    return RTNType::Good;
}


template <typename T>
auto speck::read_whole_file( const char* filename ) -> std::pair<std::unique_ptr<T[]>, size_t>
{
    std::FILE* file = std::fopen( filename, "rb" );
    if( !file )
        return {nullptr, 0};

    std::fseek( file, 0, SEEK_END );
    const size_t file_size = std::ftell( file );
    const size_t num_vals  = file_size / sizeof(T);
    std::fseek( file, 0, SEEK_SET );

    auto buf = std::make_unique<T[]>( num_vals );
    size_t nread  = std::fread( buf.get(), sizeof(T), num_vals, file );
    std::fclose( file );
    if( nread != num_vals )
        return {nullptr, 0};
    else
        return {std::move(buf), num_vals};
}
template auto speck::read_whole_file( const char* ) -> speck::smart_buffer_f;
template auto speck::read_whole_file( const char* ) -> speck::smart_buffer_d;
template auto speck::read_whole_file( const char* ) -> speck::smart_buffer_uint8;


auto speck::write_n_bytes( const char* filename, size_t n_bytes, const void* buffer ) -> RTNType
{
    std::FILE* f = std::fopen( filename, "wb" );
    if( !f ) {
        return RTNType::IOError;
    }
    if( std::fwrite(buffer, 1, n_bytes, f) != n_bytes ) {
        std::fclose( f );
        return RTNType::IOError;
    }
    std::fclose( f );
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
        T buf[ stride_size ];
        for( size_t i = 0; i < stride_size; i++ ) {
            const size_t idx = stride_i * stride_size + i;
            auto diff = std::abs( arr1[idx] - arr2[idx] );
            linfty    = std::max( linfty, diff );
            buf[i]    = diff * diff;
        }
        sum_vec   [ stride_i ] = speck::kahan_summation( buf, stride_size );
        linfty_vec[ stride_i ] = linfty;
    }

    //
    // Calculate summation and l-infty of the remaining elements
    //
    T last_linfty = 0.0;
    T last_buf[ stride_size ]; // must be enough for `remainder_size` elements
    for( size_t i = 0; i < remainder_size; i++ ) {
        const size_t idx = stride_size * num_of_strides + i;
        auto diff   = std::abs( arr1[idx] - arr2[idx] );
        last_linfty = std::max( last_linfty, diff );
        last_buf[i] = diff * diff;
    }
    sum_vec   [ num_of_strides ] = speck::kahan_summation( last_buf, remainder_size );
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
    const auto avg = speck::kahan_summation( sum_vec.data(), sum_vec.size() ) / T(len);
    *rmse    = std::sqrt( avg );
    auto range_sq = *minmax.first - *minmax.second;
    range_sq *= range_sq;
    *psnr = T(-10.0) * std::log10( avg / range_sq );
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



template <typename T>
auto speck::empty_buf( const std::pair<T, size_t>& buf ) -> bool
{
    return (buf.first == nullptr || buf.second == 0);
}
template auto speck::empty_buf( const smart_buffer_d&     ) -> bool;
template auto speck::empty_buf( const smart_buffer_f&     ) -> bool;
template auto speck::empty_buf( const smart_buffer_uint8& ) -> bool;
template auto speck::empty_buf( const std::pair<const buffer_type_d&, size_t>& ) -> bool;


template <typename T>
auto speck::size_is( const  std::pair<T, size_t>& buf,
                     size_t expected_size ) -> bool
{
    return (buf.first != nullptr && buf.second == expected_size);
}
template auto speck::size_is( const smart_buffer_d&,     size_t ) -> bool;
template auto speck::size_is( const smart_buffer_f&,     size_t ) -> bool;
template auto speck::size_is( const smart_buffer_uint8&, size_t ) -> bool;
template auto speck::size_is( const std::pair<const buffer_type_d&, size_t>&, size_t ) -> bool;


auto speck::chunk_volume( const std::array<size_t, 3>& vol_dim, 
                          const std::array<size_t, 3>& chunk_dim )
                          -> std::vector< std::array<size_t, 6> >
{
    for( size_t i = 0; i < 3; i++ )
        if( vol_dim[i] < chunk_dim[i] )
            return {};

    // Step 1: figure out how many segments are there along each axis.
    auto n_segs = std::array<size_t, 3>();
    for( size_t i = 0; i < 3; i++ ) {
        n_segs[i] = vol_dim[i] / chunk_dim[i];
        if( (vol_dim[i] % chunk_dim[i]) > (chunk_dim[i] / 2) )
            n_segs[i]++;
    }

    // Step 2: calculate the starting indices of each segment along each axis.
    auto x_tics = std::vector<size_t>( n_segs[0] + 1 );
    for( size_t i = 0; i < n_segs[0]; i++ )
        x_tics[i] = i * chunk_dim[0];
    x_tics[n_segs[0]] = vol_dim[0];

    auto y_tics = std::vector<size_t>( n_segs[1] + 1 );
    for( size_t i = 0; i < n_segs[1]; i++ )
        y_tics[i] = i * chunk_dim[1];
    y_tics[n_segs[1]] = vol_dim[1];

    auto z_tics = std::vector<size_t>( n_segs[2] + 1 );
    for( size_t i = 0; i < n_segs[2]; i++ )
        z_tics[i] = i * chunk_dim[2];
    z_tics[n_segs[2]] = vol_dim[2];

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






