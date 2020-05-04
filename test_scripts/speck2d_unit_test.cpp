
#include "SPECK2D.h"
#include "CDF97.h"
#include <cstdlib>
#include "gtest/gtest.h"

namespace
{

extern "C"  // C Function calls, and don't include the C header!
{
    int sam_read_n_bytes( const char*, size_t, void* );
    int sam_get_statsd( const double* arr1, const double* arr2, size_t len,
                        double* rmse,       double* lmax,   double* psnr,
                        double* arr1min,    double* arr1max            );
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

    double get_psnr() const
    {
        return m_psnr;
    }

    double get_lmax() const
    {
        return m_lmax;
    }

    // Execute the compression/decompression pipeline. Return 0 on success
    int execute( float cratio )
    {
        const size_t  total_vals = m_dim_x * m_dim_y;

        // Let's read in binaries as 4-byte floats
        std::unique_ptr<float[]> in_buf( new float[ total_vals ] );
        if( sam_read_n_bytes( m_input_name.c_str(), sizeof(float) * total_vals, in_buf.get() ) )
        {
            std::cerr << "Input read error!" << std::endl;
            return 1;
        }

        // Take input to go through DWT.
        speck::CDF97 cdf;
        cdf.set_dims( m_dim_x, m_dim_y );
        cdf.copy_data( in_buf, total_vals );
        cdf.dwt2d();

        // Do a speck encoding
        speck::SPECK2D encoder;
        encoder.set_dims( m_dim_x, m_dim_y );
        encoder.set_image_mean( cdf.get_mean() );
        encoder.copy_coeffs( cdf.get_read_only_data(), m_dim_x * m_dim_y );
        const size_t total_bits = size_t(32.0f * total_vals / cratio);
        encoder.set_bit_budget( total_bits );
        encoder.encode();
        if( encoder.write_to_disk( m_output_name ) )
        {
            std::cerr << "Write bitstream to disk error!" << std::endl;
            return 1;
        }

        // Do a speck decoding
        speck::SPECK2D decoder;
        if( decoder.read_from_disk( m_output_name ) )
        {
            std::cerr << "Read bitstream from disk error!" << std::endl;
            return 1;
        }
        decoder.set_bit_budget( total_bits );
        decoder.decode();

        speck::CDF97 idwt;
        size_t dim_x_r, dim_y_r;
        decoder.get_dims( dim_x_r, dim_y_r );
        idwt.set_dims( dim_x_r, dim_y_r );
        idwt.set_mean( decoder.get_image_mean() );
        idwt.take_data( decoder.release_coeffs_double() );
        idwt.idwt2d();

        // Compare the result with the original input in double precision
        std::unique_ptr<double[]> in_bufd( new double[ total_vals ] );
        for( size_t i = 0; i < total_vals; i++ )
            in_bufd[i] = in_buf[i];
        double rmse, lmax, psnr, arr1min, arr1max;
        sam_get_statsd( in_bufd.get(), idwt.get_read_only_data().get(), 
                        total_vals, &rmse, &lmax, &psnr, &arr1min, &arr1max );
        // Uncomment the following lines to have these statistics printed.
        //printf("Sam: rmse = %f, lmax = %f, psnr = %fdB, orig_min = %f, orig_max = %f\n", 
        //        rmse, lmax, psnr, arr1min, arr1max );
        m_psnr = psnr;
        m_lmax = lmax;

        return 0;
    }


private:
    std::string m_input_name;
    size_t m_dim_x, m_dim_y;
    std::string m_output_name = "output.tmp";
    double m_psnr, m_lmax;
};


TEST( speck2d, lena )
{
    speck_tester tester( "../test_data/lena512.float", 512, 512 );

    tester.execute( 8.0f );
    double psnr = tester.get_psnr();
    double lmax = tester.get_lmax();
    EXPECT_GT( psnr, 54.2830 );
    EXPECT_LT( lmax,  2.2361 );

    tester.execute( 16.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 43.2870 );
    EXPECT_LT( lmax,  7.1736 );

    tester.execute( 32.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 38.8008 );
    EXPECT_LT( lmax, 14.4871 );

    tester.execute( 64.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 35.6299 );
    EXPECT_LT( lmax, 37.109243 );
}


TEST( speck2d, odd_dim_image )
{
    speck_tester tester( "../test_data/90x90.float", 90, 90 );

    tester.execute( 8.0f );
    double psnr = tester.get_psnr();
    double lmax = tester.get_lmax();
    EXPECT_GT( psnr, 58.7325 );
    EXPECT_LT( lmax,  0.7588 );

    tester.execute( 16.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 46.7979 );
    EXPECT_LT( lmax,  2.9545 );

    tester.execute( 32.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 40.05257 );
    EXPECT_LT( lmax,  6.25197 );

    tester.execute( 64.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 34.9631 );
    EXPECT_LT( lmax, 18.644617 );
}


TEST( speck2d, small_data_range )
{
    speck_tester tester( "../test_data/vorticity.512_512", 512, 512 );

    tester.execute( 8.0f );
    double psnr = tester.get_psnr();
    double lmax = tester.get_lmax();
    EXPECT_GT( psnr, 71.289441 );
    EXPECT_LT( lmax, 0.000002  );

    tester.execute( 16.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 59.666803 );
    EXPECT_LT( lmax, 0.0000084 );

    tester.execute( 32.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 52.396355 );
    EXPECT_LT( lmax, 0.0000213 );

    tester.execute( 64.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 46.906873 );
    EXPECT_LT( lmax, 0.0000475  );
}


}
