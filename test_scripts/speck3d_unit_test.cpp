#include "SPECK3D_Compressor.h"
#include "SPECK3D_Decompressor.h"

#include "SPECK3D_OMP_C.h"
#include "SPECK3D_OMP_D.h"

#include <cstring>
#include "gtest/gtest.h"

namespace
{

using speck::RTNType;

// Create a class that executes the entire pipeline, and calculates the error metrics
// This class tests objects SPECK3D_Compressor and SPECK3D_Decompressor
class speck_tester
{
public:
    speck_tester( const char* in, speck::dims_type dims )
    {
        m_input_name = in;
        m_dims       = dims;
    }

    float get_psnr() const
    {
        return m_psnr;
    }

    float get_lmax() const
    {
        return m_lmax;
    }

    //
    // Execute the compression/decompression pipeline. Return 0 on success
    //

#ifdef QZ_TERM
    int execute( int32_t qz_level, double tol )
#else
    int execute( float bpp )
#endif

    {
        // Reset lmax and psnr
        m_psnr = 0.0;
        m_lmax = 1000.0;
        
        const size_t  total_vals = m_dims[0] * m_dims[1] * m_dims[2];

        //
        // Use a compressor 
        //
        auto in_buf = speck::read_whole_file<float>( m_input_name.c_str() );
        if( in_buf.size() != total_vals )
            return 1;
        SPECK3D_Compressor compressor;
        if( compressor.copy_data( in_buf.data(), total_vals, m_dims ) != RTNType::Good )
            return 1;

#ifdef QZ_TERM
        compressor.set_qz_level( qz_level );
        compressor.set_tolerance( tol );
#else
        compressor.set_bpp( bpp );
#endif

        if( compressor.compress() != RTNType::Good )
            return 1;
        auto stream = compressor.release_encoded_bitstream();
        if( stream.empty() )
            return 1;

        //
        // Use a decompressor 
        //
        SPECK3D_Decompressor decompressor;
        if( decompressor.use_bitstream( stream.data(), stream.size() ) != RTNType::Good )
            return 1;
        if( decompressor.decompress() != RTNType::Good )
            return 1;
        auto vol = decompressor.get_data<float>();
        if( vol.size() != total_vals )
            return 1;

        //
        // Compare results
        //
        float rmse, lmax, psnr, arr1min, arr1max;
        speck::calc_stats( in_buf.data(), vol.data(), total_vals, 8,
                           rmse, lmax, psnr, arr1min, arr1max );
        m_psnr = psnr;
        m_lmax = lmax;

        return 0;
    }


private:
    std::string m_input_name;
    speck::dims_type m_dims = {0, 0, 0};
    std::string m_output_name = "output.tmp";
    float m_psnr, m_lmax;
};


// Create a class that executes the entire pipeline, and calculates the error metrics
// This class tests objects SPECK3D_OMP_C and SPECK3D_OMP_D
class speck_tester_omp
{
public:
    speck_tester_omp( const char* in, speck::dims_type dims, size_t num_t )
    {
        m_input_name = in;
        m_dims       = dims;
        m_num_t      = num_t;
    }

    void prefer_chunk_dims( speck::dims_type dims )
    {
      m_chunk_dims = dims;
    }

    float get_psnr() const
    {
        return m_psnr;
    }

    float get_lmax() const
    {
        return m_lmax;
    }

    //
    // Execute the compression/decompression pipeline. Return 0 on success
    //

#ifdef QZ_TERM
    int execute( int32_t qz_level, double tol )
#else
    int execute( float bpp )
#endif

    {
        // Reset lmax and psnr
        m_psnr = 0.0;
        m_lmax = 1000.0;
        
        const size_t  total_vals = m_dims[0] * m_dims[1] * m_dims[2];

        //
        // Use a compressor 
        //
        auto in_buf = speck::read_whole_file<float>( m_input_name.c_str() );
        if( in_buf.size() != total_vals )
            return 1;

        SPECK3D_OMP_C compressor;
        compressor.set_dims( m_dims );
        compressor.prefer_chunk_dims( m_chunk_dims );
        compressor.set_num_threads( m_num_t );

        if( compressor.use_volume( in_buf.data(), total_vals ) != RTNType::Good )
            return 1;

#ifdef QZ_TERM
        compressor.set_qz_level( qz_level );
        compressor.set_tolerance( tol );
#else
        compressor.set_bpp( bpp );
#endif

        if( compressor.compress() != RTNType::Good )
            return 1;
        auto stream = compressor.get_encoded_bitstream();
        if( stream.empty() )
            return 1;

        //
        // Use a decompressor 
        //
        SPECK3D_OMP_D decompressor;
        decompressor.set_num_threads( m_num_t );
        if( decompressor.use_bitstream( stream.data(), stream.size() ) != RTNType::Good )
            return 1;
        if( decompressor.decompress( stream.data() ) != RTNType::Good )
            return 1;
        auto vol = decompressor.get_data<float>();
        if( vol.size() != total_vals )
            return 1;

        //
        // Compare results
        //
        const size_t nbytes = sizeof(float) * total_vals;
        auto orig = std::make_unique<float[]>(total_vals);
        if( speck::read_n_bytes( m_input_name.c_str(), nbytes, orig.get() ) != 
            speck::RTNType::Good )
            return 1;

        float rmse, lmax, psnr, arr1min, arr1max;
        speck::calc_stats( orig.get(), vol.data(), total_vals, 8,
                           rmse, lmax, psnr, arr1min, arr1max );
        m_psnr = psnr;
        m_lmax = lmax;

        return 0;
    }

private:
    std::string m_input_name;
    speck::dims_type m_dims = {0, 0, 0};
    speck::dims_type m_chunk_dims = {64, 64, 64};
    std::string m_output_name = "output.tmp";
    float m_psnr, m_lmax;
    size_t m_num_t;
};

//
// Test constant fields.
//
TEST( speck3d_constant, one_chunk )
{
  speck_tester tester( "../test_data/const32x20x16.float", {32, 20, 16} );
#ifdef QZ_TERM
  auto rtn = tester.execute( -1, 1 );
#else
  auto rtn = tester.execute( 1.0f ); 
#endif
  EXPECT_EQ( rtn, 0 );
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  auto infty = std::numeric_limits<float>::infinity();
  EXPECT_EQ( psnr, infty );
  EXPECT_EQ( lmax, 0.0f );
}

TEST( speck3d_constant, omp_chunks )
{
  speck_tester_omp tester( "../test_data/const32x32x59.float", {32, 32, 59}, 8 );
  tester.prefer_chunk_dims({32, 32, 32});
#ifdef QZ_TERM
  auto rtn = tester.execute( -1, 1 );
#else
  auto rtn = tester.execute( 1.0f ); 
#endif
  EXPECT_EQ( rtn, 0 );
  auto psnr = tester.get_psnr();
  auto lmax = tester.get_lmax();
  auto infty = std::numeric_limits<float>::infinity();
  EXPECT_EQ( psnr, infty );
  EXPECT_EQ( lmax, 0.0f );
}


#ifdef QZ_TERM
//
// Error bound mode
//
TEST( speck3d_qz_term, large_tolerance )
{
    const float tol = 1.0;
    speck_tester tester( "../test_data/wmag128.float", {128, 128, 128} );
    auto rtn = tester.execute( 2, tol );
    EXPECT_EQ( rtn, 0 );
    float psnr = tester.get_psnr();
    float lmax = tester.get_lmax();
    EXPECT_GT( psnr, 5.76309120e+01 );
    EXPECT_LT( psnr, 5.76309166e+01 );
    EXPECT_LE( lmax, tol );

    rtn = tester.execute( -1, tol );
    EXPECT_EQ( rtn, 0 );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 6.5578315e+01 );
    EXPECT_LT( psnr, 6.5578317e+01 );
    EXPECT_LT( lmax, tol );

    rtn = tester.execute( -2, tol );
    EXPECT_EQ( rtn, 0 );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 7.2108818e+01 );
    EXPECT_LT( psnr, 7.2108826e+01 );
    EXPECT_LT( lmax, 5.623770e-01 );
}
TEST( speck3d_qz_term, small_tolerance )
{
    const double tol = 0.07;
    speck_tester tester( "../test_data/wmag128.float", {128, 128, 128} );
    auto rtn = tester.execute( -3, tol );
    EXPECT_EQ( rtn, 0 );
    float psnr = tester.get_psnr();
    float lmax = tester.get_lmax();
    EXPECT_GT( psnr, 8.1451720e+01 );
    EXPECT_LT( psnr, 8.1451722e+01 );
    EXPECT_LT( lmax, 6.9999696e-02 );

    rtn = tester.execute( -5, tol );
    EXPECT_EQ( rtn, 0 );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 9.1753349e+01 );
    EXPECT_LT( psnr, 9.1753350e+01 );
    EXPECT_LE( lmax, 5.3462983e-02 );
}
TEST( speck3d_qz_term, narrow_data_range)
{
    speck_tester tester( "../test_data/vorticity.128_128_41", {128, 128, 41} );
    auto rtn = tester.execute( -16, 3e-5 );
    EXPECT_EQ( rtn, 0 );
    float psnr = tester.get_psnr();
    float lmax = tester.get_lmax();
    EXPECT_GT( psnr, 42.293655 );
    EXPECT_LT( psnr, 42.293656 );
    EXPECT_LT( lmax, 2.983993e-05 );

    rtn = tester.execute( -18, 7e-6 );
    EXPECT_EQ( rtn, 0 );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 5.05222282e+01 );
    EXPECT_LT( psnr, 5.05222283e+01 );
    EXPECT_LT( lmax, 6.997870e-06 );
}
TEST( speck3d_qz_term_omp, narrow_data_range)
{
    // We specify to use 1 thread to make sure that object re-use has no side effects.
    // The next set of tests will use multiple threads.
    speck_tester_omp tester( "../test_data/vorticity.128_128_41", {128, 128, 41}, 1 );
    auto rtn = tester.execute( -16, 3e-5 );
    EXPECT_EQ( rtn, 0 );
    float psnr = tester.get_psnr();
    float lmax = tester.get_lmax();
    EXPECT_GT( psnr, 4.22075080e+01 );
    EXPECT_LT( psnr, 4.22075081e+01 );
    EXPECT_LT( lmax, 2.987783e-05 );

    rtn = tester.execute( -18, 7e-6 );
    EXPECT_EQ( rtn, 0 );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 50.486732 );
    EXPECT_LT( psnr, 50.486733 );
    EXPECT_LT( lmax, 6.991625e-06 );
}
TEST( speck3d_qz_term_omp, small_tolerance )
{
    speck_tester_omp tester( "../test_data/wmag128.float", {128, 128, 128}, 7 );
    auto rtn = tester.execute( -3, 0.07);
    EXPECT_EQ( rtn, 0 );
    float psnr = tester.get_psnr();
    float lmax = tester.get_lmax();
    EXPECT_GT( psnr, 8.1441611e+01 );
    EXPECT_LT( psnr, 8.1441613e+01 );
    EXPECT_LT( lmax, 6.9999696e-02 );

    rtn = tester.execute( -5, 0.05 );
    EXPECT_EQ( rtn, 0 );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 9.1716300e+01 );
    EXPECT_LT( psnr, 9.1716302e+01 );
    EXPECT_LT( lmax, 4.9962998e-02 );
}

#else
//
// fixed-size mode
//
TEST( speck3d_bit_rate, small )
{
    speck_tester tester( "../test_data/wmag17.float", {17, 17, 17} );

    tester.execute( 4.0f );
    float psnr = tester.get_psnr();
    float lmax = tester.get_lmax();
    EXPECT_GT( psnr, 5.337071e+01 );
    EXPECT_LT( psnr, 5.337072e+01 );
    EXPECT_LT( lmax, 1.4303418e+0 );

    tester.execute( 2.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 4.1785873e+01 ); // Satisfy mac and linux
    EXPECT_LT( psnr, 4.1785882e+01 ); // Satisfy mac and linux
    EXPECT_LT( lmax, 6.3229772e+0 );

    tester.execute( 1.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 3.5392981e+01 );
    EXPECT_LT( psnr, 3.5392983e+01 );
    EXPECT_LT( lmax, 1.0009356e+01 );
}

TEST( speck3d_bit_rate, big )
{
    speck_tester tester( "../test_data/wmag128.float", {128, 128, 128} );

    tester.execute( 2.0f );
    float psnr = tester.get_psnr();
    float lmax = tester.get_lmax();
    EXPECT_GT( psnr, 5.484075e+01 );
    EXPECT_LT( psnr, 5.484076e+01 );
    EXPECT_LT( lmax, 4.9368744e+00 );

    tester.execute( 1.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 4.800989e+01 );
    EXPECT_LT( psnr, 4.800990e+01 );
    EXPECT_LT( lmax, 1.0140460e+01 );

    tester.execute( 0.5f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 4.3428554e+01 );
    EXPECT_LT( psnr, 4.3428555e+01 );
    EXPECT_LT( lmax, 2.2674592e+01 );

    tester.execute( 0.25f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 3.9851757e+01 );
    EXPECT_LT( psnr, 3.9851758e+01 );
    EXPECT_LT( lmax, 3.9112084e+01 );
}

TEST( speck3d_bit_rate, narrow_data_range )
{
    speck_tester tester( "../test_data/vorticity.128_128_41", {128, 128, 41} );

    tester.execute( 4.0f );
    float psnr = tester.get_psnr();
    float lmax = tester.get_lmax();
    EXPECT_GT( psnr, 69.043647 );
    EXPECT_LT( psnr, 69.043656 );
    EXPECT_LT( lmax, 9.103715e-07 );

    tester.execute( 2.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 56.787048 );
    EXPECT_LT( psnr, 56.787049 );
    EXPECT_LT( lmax, 4.199554e-06 );

    tester.execute( 1.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 49.777523 );
    EXPECT_LT( psnr, 49.777524 );
    EXPECT_LT( lmax, 1.002031e-05 );

    tester.execute( 0.5f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 45.207603 );
    EXPECT_LT( psnr, 45.207604 );
    EXPECT_LT( lmax, 0.000024 );

    tester.execute( 0.25f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 41.7556152 ); // to satisfy both mac and linux
    EXPECT_LT( psnr, 41.7556191 ); // to satisfy both mac and linux
    EXPECT_LT( lmax, 3.329716e-05 );
}

TEST( speck3d_bit_rate_omp, narrow_data_range )
{
    speck_tester_omp tester( "../test_data/vorticity.128_128_41", {128, 128, 41}, 1 );

    tester.execute( 4.0f );
    float psnr = tester.get_psnr();
    float lmax = tester.get_lmax();
    EXPECT_GT( psnr, 67.823394 );
    EXPECT_LT( psnr, 67.823395 );
    EXPECT_LT( lmax, 1.108655e-06 );

    tester.execute( 1.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 4.891277e+01 );
    EXPECT_LT( psnr, 4.891278e+01 );
    EXPECT_LT( lmax, 1.396333e-05 );
}

TEST( speck3d_bit_rate_omp, big )
{
    speck_tester_omp tester( "../test_data/wmag128.float", {128, 128, 128}, 8 );

    tester.execute( 2.0f );
    float psnr = tester.get_psnr();
    float lmax = tester.get_lmax();
    EXPECT_GT( psnr, 5.3812061e+01 ); // satisfy both mac and linux
    EXPECT_LT( psnr, 5.3812066e+01 ); // satisfy both mac and linux
    EXPECT_LT( lmax, 7.6953269e+00 );

    tester.execute( 1.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 4.7230952e+01 );
    EXPECT_LT( psnr, 4.7230954e+01 );
    EXPECT_LT( lmax, 1.6300519e+01 );
}

#endif

}
