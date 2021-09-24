#include "SPECK2D_Compressor.h"
#include "SPECK2D_Decompressor.h"
#include <cstdlib>
#include "gtest/gtest.h"

using sperr::RTNType;

namespace
{

// Create a class that executes the entire pipeline, and calculates the error metrics
class sperr_tester
{
public:
    sperr_tester( const char* in, size_t x, size_t y )
    {
        m_input_name = in;
        m_dim_x      = x;
        m_dim_y      = y;
    }

    float get_psnr() const
    {
        return m_psnr;
    }

    float get_lmax() const
    {
        return m_lmax;
    }

    // Execute the compression/decompression pipeline. Return 0 on success
    int execute( float bpp )
    {
        // Reset lmax and psnr
        m_psnr = 0.0;
        m_lmax = 10000.0;

        const size_t  total_vals = m_dim_x * m_dim_y;
        auto in_buf = sperr::read_whole_file<float>( m_input_name.c_str() );
        if( in_buf.size() != total_vals )
            return 1;

        //
        // Use a compressor
        //
        SPECK2D_Compressor compressor;
        if( compressor.copy_data(in_buf.data(), total_vals, {m_dim_x, m_dim_y, 1}) 
            != RTNType::Good)
            return 1;
        if( compressor.set_bpp( bpp ) != RTNType::Good )
            return 1;
        if( compressor.compress() != RTNType::Good )
            return 1;
        auto bitstream = compressor.release_encoded_bitstream();

        //
        // Then use a decompressor
        //
        SPECK2D_Decompressor decompressor;
        if( decompressor.use_bitstream( bitstream.data(), bitstream.size() ) != RTNType::Good )
            return 1;
        if( decompressor.decompress() != RTNType::Good )
            return 1;
        auto slice = decompressor.get_data<float>();
        if( slice.size() != total_vals )
            return 1;

        //
        // Compare results 
        //
        float rmse, lmax, psnr, arr1min, arr1max;
        sperr::calc_stats( in_buf.data(), slice.data(), total_vals, 8,
                           rmse, lmax, psnr, arr1min, arr1max );
        m_psnr = psnr;
        m_lmax = lmax;

        return 0;
    }


private:
    std::string m_input_name;
    size_t m_dim_x, m_dim_y;
    std::string m_output_name = "output.tmp";
    float m_psnr, m_lmax;
};


TEST( sperr2d, lena )
{
    sperr_tester tester( "../test_data/lena512.float", 512, 512 );

    tester.execute( 4.0f );
    float psnr = tester.get_psnr();
    float lmax = tester.get_lmax();
    EXPECT_GT( psnr, 54.2830 );
    EXPECT_LT( lmax,  2.2361 );

    tester.execute( 2.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 43.2870 );
    EXPECT_LT( lmax,  7.1736 );

    tester.execute( 1.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 38.8008 );
    EXPECT_LT( lmax, 14.4871 );

    tester.execute( 0.5f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 35.6299 );
    EXPECT_LT( lmax, 37.109243 );
}


TEST( sperr2d, odd_dim_image )
{
    sperr_tester tester( "../test_data/90x90.float", 90, 90 );

    tester.execute( 4.0f );
    float psnr = tester.get_psnr();
    float lmax = tester.get_lmax();
    EXPECT_GT( psnr, 58.7325 );
    EXPECT_LT( lmax,  0.7588 );

    tester.execute( 2.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 46.7979 );
    EXPECT_LT( lmax,  2.9545 );

    tester.execute( 1.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 40.05257 );
    EXPECT_LT( lmax,  6.25197 );

    tester.execute( 0.5f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 34.9631 );
    EXPECT_LT( lmax, 18.644617 );
}


TEST( sperr2d, small_data_range )
{
    sperr_tester tester( "../test_data/vorticity.512_512", 512, 512 );

    tester.execute( 4.0f );
    float psnr = tester.get_psnr();
    float lmax = tester.get_lmax();
    EXPECT_GT( psnr, 71.289436 ); // The big difference is to 
    EXPECT_LT( psnr, 71.289444 ); // accomodate clang and gcc.
    EXPECT_LT( lmax, 0.000002  );

    tester.execute( 2.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 59.666803 );
    EXPECT_LT( lmax, 0.0000084 );

    tester.execute( 1.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 52.396354 );
    EXPECT_LT( lmax, 0.0000213 );

    tester.execute( 0.5f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 46.906871 );
    EXPECT_LT( lmax, 0.0000475  );
}


}
