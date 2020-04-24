
#include "SPECK3D.h"
#include "CDF97.h"
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
    speck_tester( const char* in, size_t x, size_t y, size_t z )
    {
        m_input_name = in;
        m_dim_x      = x;
        m_dim_y      = y;
        m_dim_z      = z;
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
        const size_t  total_vals = m_dim_x * m_dim_y * m_dim_z;

        // Let's read in binaries as 4-byte floats
        std::unique_ptr<float[]> in_buf( new float[ total_vals ] );
        if( sam_read_n_bytes( m_input_name.c_str(), sizeof(float) * total_vals, in_buf.get() ) )
        {
            std::cerr << "Input read error!" << std::endl;
            return 1;
        }

        // Take input to go through DWT.
        speck::CDF97 cdf;
        cdf.set_dims( m_dim_x, m_dim_y, m_dim_z );
        cdf.copy_data( in_buf, total_vals );
        cdf.dwt3d();

        // Do a speck encoding
        speck::SPECK3D encoder;
        encoder.set_dims( m_dim_x, m_dim_y, m_dim_z );
        encoder.set_image_mean( cdf.get_mean() );
        encoder.copy_coeffs( cdf.get_read_only_data(), total_vals );
        const size_t total_bits = size_t(32.0f * total_vals / cratio);
        encoder.set_bit_budget( total_bits );
        encoder.encode();
        if( encoder.write_to_disk( m_output_name ) )
        {
            std::cerr << "Write bitstream to disk error!" << std::endl;
            return 1;
        }

        // Do a speck decoding
        speck::SPECK3D decoder;
        if( decoder.read_from_disk( m_output_name ) )
        {
            std::cerr << "Read bitstream from disk error!" << std::endl;
            return 1;
        }
        decoder.set_bit_budget( total_bits );
        decoder.decode();

        speck::CDF97 idwt;
        size_t dim_x_r, dim_y_r, dim_z_r;
        decoder.get_dims( dim_x_r, dim_y_r, dim_z_r );
        idwt.set_dims( dim_x_r, dim_y_r, dim_z_r );
        idwt.set_mean( decoder.get_image_mean() );
        idwt.take_data( decoder.release_coeffs_double() );
        idwt.idwt3d();

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
    size_t m_dim_x, m_dim_y, m_dim_z;
    std::string m_output_name = "output.tmp";
    double m_psnr, m_lmax;
};

TEST( speck3d, small )
{
    speck_tester tester( "../test_data/wmag17.float", 17, 17, 17 );

    tester.execute( 8.0f );
    double psnr = tester.get_psnr();
    double lmax = tester.get_lmax();
    EXPECT_GT( psnr, 52.559024 );
    EXPECT_LT( lmax,  1.432911 );

    tester.execute( 32.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 34.71057 );
    EXPECT_LT( lmax, 12.45794 );

    tester.execute( 64.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 30.10208 );
    EXPECT_LT( lmax, 27.40783 );

    tester.execute( 128.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 26.62448 );
    EXPECT_LT( lmax, 36.11478 );
}

TEST( speck3d, big )
{
    speck_tester tester( "../test_data/wmag128.float", 128, 128, 128 );

    tester.execute( 16.0f );
    double psnr = tester.get_psnr();
    double lmax = tester.get_lmax();
    EXPECT_GT( psnr, 54.040208 );
    EXPECT_LT( lmax,  4.879152 );

    tester.execute( 32.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 47.27210 );
    EXPECT_LT( lmax, 15.96790 );

    tester.execute( 64.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 42.68343 );
    EXPECT_LT( lmax, 25.18307 );

    tester.execute( 128.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 39.19998 );
    EXPECT_LT( lmax, 44.29733 );
}


}
