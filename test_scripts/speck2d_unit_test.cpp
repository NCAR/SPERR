#include "SPECK2D_Compressor.h"
#include "SPECK2D_Decompressor.h"
#include <cstdlib>
#include "gtest/gtest.h"

using speck::RTNType;

namespace
{

extern "C"  // C Function calls, and don't include the C header!
{
    int sam_read_n_bytes( const char*, size_t, void* );
    int sam_get_statsf( const float* arr1, const float* arr2, size_t len,
                        float* rmse,       float* lmax,   float* psnr,
                        float* arr1min,    float* arr1max            );
}

// Create a class that executes the entire pipeline, and calculates the error metrics
class speck_tester
{
public:
    speck_tester( const char* in, size_t x, size_t y )
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

        //
        // Use a compressor
        //
        SPECK2D_Compressor compressor( m_dim_x, m_dim_y );
        if( compressor.read_floats( m_input_name.c_str() ) != RTNType::Good )
            return 1;
        if( compressor.set_bpp( bpp ) != RTNType::Good )
            return 1;
        if( compressor.compress() != RTNType::Good )
            return 1;
        if( compressor.write_bitstream( m_output_name.c_str() ) != RTNType::Good )
            return 1;

        //
        // Then use a decompressor
        //
        SPECK2D_Decompressor decompressor;
        if( decompressor.read_bitstream( m_output_name.c_str() ) != RTNType::Good )
            return 1;
        if( decompressor.decompress() != RTNType::Good )
            return 1;
        auto slice = decompressor.get_decompressed_slice_f();
        if( slice.first == nullptr || slice.second != total_vals )
            return 1;

        //
        // Compare results 
        //

        auto orig = speck::unique_malloc<float>( total_vals );
        if( sam_read_n_bytes( m_input_name.c_str(), 4 * total_vals, orig.get() ) )
            return 1;
        float rmse, lmax, psnr, arr1min, arr1max;
        if( sam_get_statsf( orig.get(), slice.first.get(), total_vals,
                            &rmse, &lmax, &psnr, &arr1min, &arr1max ) )
            return 1;
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


TEST( speck2d, lena )
{
    speck_tester tester( "../test_data/lena512.float", 512, 512 );

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


TEST( speck2d, odd_dim_image )
{
    speck_tester tester( "../test_data/90x90.float", 90, 90 );

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


TEST( speck2d, small_data_range )
{
    speck_tester tester( "../test_data/vorticity.512_512", 512, 512 );

    tester.execute( 4.0f );
    float psnr = tester.get_psnr();
    float lmax = tester.get_lmax();
    EXPECT_GT( psnr, 71.289441 );
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
