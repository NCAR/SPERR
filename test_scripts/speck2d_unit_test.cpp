
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
        cdf.copy_data( in_buf.get() );
        cdf.dwt2d();

        // Do a speck encoding
        speck::SPECK2D encoder;
        encoder.assign_dims( m_dim_x, m_dim_y );
        encoder.take_coeffs( cdf.release_data() );
        const size_t header_size  = 18;     // bytes
        const size_t total_bits   = size_t(32.0f * total_vals / cratio) + header_size * 8;
        encoder.assign_bit_budget( total_bits );
        encoder.encode();

        // Write to file and read it back
        if( speck::output_speck2d( m_dim_x, m_dim_y, cdf.get_mean(), encoder.get_max_coeff_bits(), 
                                   encoder.get_read_only_bitstream(), m_output_name )            )
        {
            std::cerr << "Write bitstream to disk error!" << std::endl;
            return 1;
        }
        size_t dim_x_r, dim_y_r;
        double mean_r;
        uint16_t max_bits_r;
        std::vector<bool> bits_r;
        if( speck::input_speck2d( dim_x_r, dim_y_r, mean_r, max_bits_r, bits_r, m_output_name ) )
        {
            std::cerr << "Read bitstream from disk error!" << std::endl;
            return 1;
        }
        
        // Do a speck decoding
        speck::SPECK2D decoder;
        decoder.assign_dims( dim_x_r, dim_y_r );
        decoder.assign_max_coeff_bits( max_bits_r );
        decoder.take_bitstream( bits_r );
        decoder.assign_bit_budget( total_bits );
        decoder.decode();

        speck::CDF97 idwt;
        idwt.set_dims( dim_x_r, dim_y_r );
        idwt.set_mean( cdf.get_mean() );
        idwt.take_data( decoder.release_coeffs() );
        idwt.idwt2d();

        // Compare the result with the original input in double precision
        std::unique_ptr<double[]> in_bufd( new double[ total_vals ] );
        for( size_t i = 0; i < total_vals; i++ )
            in_bufd[i] = in_buf[i];
        double rmse, lmax, psnr, arr1min, arr1max;
        sam_get_statsd( in_bufd.get(), idwt.get_read_only_data(), total_vals, 
                        &rmse, &lmax, &psnr, &arr1min, &arr1max );
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
    EXPECT_GT( psnr, 54.2861 );
    EXPECT_LT( lmax,  2.2361 );

    tester.execute( 16.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 43.2886 );
    EXPECT_LT( lmax,  7.1736 );

    tester.execute( 32.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 38.8037 );
    EXPECT_LT( lmax, 14.4871 );

    tester.execute( 64.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 35.6353 );
    EXPECT_LT( lmax, 37.1092 );
}


TEST( speck2d, odd_dim_image )
{
    speck_tester tester( "../test_data/90x90.float", 90, 90 );

    tester.execute( 8.0f );
    double psnr = tester.get_psnr();
    double lmax = tester.get_lmax();
    EXPECT_GT( psnr, 58.8549 );
    EXPECT_LT( lmax,  0.7588 );

    tester.execute( 16.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 46.9400 );
    EXPECT_LT( lmax,  2.9545 );

    tester.execute( 32.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 40.2515 );
    EXPECT_LT( lmax,  5.8753 );

    tester.execute( 64.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 35.2209 );
    EXPECT_LT( lmax, 18.6444 );
}


}
