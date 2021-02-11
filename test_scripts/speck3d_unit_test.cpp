#include "SPECK3D_Compressor.h"
#include "SPECK3D_Decompressor.h"
#include <cstring>
#include "gtest/gtest.h"

namespace
{

using speck::RTNType;

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

    void reset( const char* in, size_t x, size_t y, size_t z )
    {
        m_input_name = in;
        m_dim_x      = x;
        m_dim_y      = y;
        m_dim_z      = z;
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
        
        const size_t  total_vals = m_dim_x * m_dim_y * m_dim_z;

        //
        // Use a compressor 
        //
        SPECK3D_Compressor compressor( m_dim_x, m_dim_y, m_dim_z );
        if( compressor.read_floats( m_input_name.c_str() ) != RTNType::Good )
            return 1;

#ifdef QZ_TERM
        compressor.set_qz_level( qz_level );
        compressor.set_tolerance( tol );
#else
        compressor.set_bpp( bpp );
#endif

        if( compressor.compress() != RTNType::Good )
            return 1;
        if( compressor.write_bitstream( m_output_name.c_str() ) != RTNType::Good )
            return 1;

        
        //
        // Use a decompressor 
        //
        SPECK3D_Decompressor decompressor;
        if( decompressor.read_bitstream( m_output_name.c_str() ) != RTNType::Good )
            return 1;
        if( decompressor.decompress() != RTNType::Good )
            return 1;
        auto vol = decompressor.get_decompressed_volume_f();
        if( vol.first == nullptr || vol.second != total_vals )
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
        speck::calc_stats( orig.get(), vol.first.get(), total_vals,
                           &rmse, &lmax, &psnr, &arr1min, &arr1max );
        m_psnr = psnr;
        m_lmax = lmax;

        return 0;
    }


private:
    std::string m_input_name;
    size_t m_dim_x, m_dim_y, m_dim_z;
    std::string m_output_name = "output.tmp";
    float m_psnr, m_lmax;
};


#ifdef QZ_TERM
TEST( speck3d_qz_term, large_tolerance )
{
    const double tol = 1.0;
    speck_tester tester( "../test_data/wmag128.float", 128, 128, 128 );
    tester.execute( 2, tol );
    float psnr = tester.get_psnr();
    float lmax = tester.get_lmax();
    EXPECT_GT( psnr, 57.629364 );
    EXPECT_LT( lmax, tol );

    tester.execute( -1, tol );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 65.498861 );
    EXPECT_LT( lmax, tol );

    tester.execute( -2, tol );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 72.025230 );
    EXPECT_LT( lmax, 0.6164713 );
}
TEST( speck3d_qz_term, small_tolerance )
{
    const double tol = 0.07;
    speck_tester tester( "../test_data/wmag128.float", 128, 128, 128 );
    tester.execute( -3, tol );
    float psnr = tester.get_psnr();
    float lmax = tester.get_lmax();
    EXPECT_GT( psnr, 81.446037 );
    EXPECT_LT( lmax, tol );

    tester.execute( -5, tol );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 91.618080 );
    EXPECT_LT( lmax, 0.0637522 );
}
TEST( speck3d_qz_term, narrow_data_range)
{
    speck_tester tester( "../test_data/vorticity.128_128_41", 128, 128, 41 );
    tester.execute( -16, 4e-5 );
    float psnr = tester.get_psnr();
    float lmax = tester.get_lmax();
    EXPECT_GT( psnr, 42.292308 );
    EXPECT_LT( lmax, 3.168651e-5 );

    tester.execute( -18, 4e-5 );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 50.513606 );
    EXPECT_LT( lmax, 8.966978e-6 );
}
#else
TEST( speck3d_bit_rate, small )
{
    speck_tester tester( "../test_data/wmag17.float", 17, 17, 17 );

    tester.execute( 4.0f );
    float psnr = tester.get_psnr();
    float lmax = tester.get_lmax();
    EXPECT_GT( psnr, 52.646259 );
    EXPECT_LT( lmax, 1.4229241 );

    tester.execute( 2.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 41.48256 );
    EXPECT_LT( lmax, 5.468170 );

    tester.execute( 1.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 34.713615 );
    EXPECT_LT( lmax, 12.496041 );

    tester.execute( 0.5f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 30.10208 );
    EXPECT_LT( lmax, 27.40783 );

    tester.execute( 0.25f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 26.62448 );
    EXPECT_LT( lmax, 36.11478 );
}


TEST( speck3d_bit_rate, big )
{
    speck_tester tester( "../test_data/wmag128.float", 128, 128, 128 );

    tester.execute( 2.0f );
    float psnr = tester.get_psnr();
    float lmax = tester.get_lmax();
    EXPECT_GT( psnr, 54.040206 );
    EXPECT_LT( lmax,  4.879152 );

    tester.execute( 1.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 47.27210 );
    EXPECT_LT( lmax, 15.96790 );

    tester.execute( 0.5f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 42.68343 );
    EXPECT_LT( lmax, 25.18307 );

    tester.execute( 0.25f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 39.19998 );
    EXPECT_LT( lmax, 44.29733 );
}


TEST( speck3d_bit_rate, narrow_data_range )
{
    speck_tester tester( "../test_data/vorticity.128_128_41", 128, 128, 41 );

    tester.execute( 4.0f );
    float psnr = tester.get_psnr();
    float lmax = tester.get_lmax();
    EXPECT_GT( psnr, 68.791297 );
    EXPECT_LT( lmax, 0.000001 );

    tester.execute( 2.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 56.628458 );
    EXPECT_LT( lmax, 0.000005 );

    tester.execute( 1.0f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 49.660499 );
    EXPECT_LT( lmax, 0.0000104 );

    tester.execute( 0.5f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 45.122822 );
    EXPECT_LT( lmax, 0.000024 );

    tester.execute( 0.25f );
    psnr = tester.get_psnr();
    lmax = tester.get_lmax();
    EXPECT_GT( psnr, 41.702354 );
    EXPECT_LT( lmax, 0.0000375 );
}
#endif

}
