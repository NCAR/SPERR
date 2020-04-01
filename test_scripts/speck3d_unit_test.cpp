
#include "SPECK3D.h"
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

// This function is part of speck::SPECK3D::m_decide_set_S_significance().
void decide_subset_significance( const speck::SPECKSet3D&            set, 
                                 const std::vector< speck::UINT >&   signif_idx,
                                 std::array<speck::Significance, 8>& sigs )
{
    for( size_t i = 0; i < sigs.size(); i++ )
        sigs[i] = speck::Significance::Insig;

    const auto detail_start_x = set.start_x + set.length_x - set.length_x / 2;
    const auto detail_start_y = set.start_y + set.length_y - set.length_y / 2;
    const auto detail_start_z = set.start_z + set.length_z - set.length_z / 2;
    for( size_t i = 0; i < signif_idx.size(); i += 3 )
    {
        auto x = signif_idx[i];
        auto y = signif_idx[i + 1];
        auto z = signif_idx[i + 2];
        size_t subband = 0;
        if( x >= detail_start_x )
            subband += 1;
        if( y >= detail_start_y )
            subband += 2;
        if( z >= detail_start_z )
            subband += 4;
        sigs[ subband ] = speck::Significance::Sig;
    }
}


TEST( speck3d, divide_in_3_axes )
{
    speck::SPECKSet3D set;
    std::vector< speck::UINT >  idx;
    std::array<speck::Significance, 8> sigs;

    // Test 1: a perfect 2x2x2 cube starting from (0, 0, 0)
    set.start_x  = 0;   set.start_y  = 0;    set.start_z  = 0;
    set.length_x = 2;   set.length_y = 2;    set.length_z = 2;
    idx.push_back(0);   idx.push_back(0);   idx.push_back(0);   // (0, 0, 0) is significant
    idx.push_back(1);   idx.push_back(1);   idx.push_back(1);   // (1, 1, 1) is significant
    decide_subset_significance( set, idx, sigs );
    EXPECT_EQ( sigs[0], speck::Significance::Sig );
    EXPECT_EQ( sigs[7], speck::Significance::Sig );
    for( int i = 1; i < 1 + 6; i++ )
        EXPECT_EQ( sigs[i], speck::Significance::Insig );

    // Test 2: a perfect 2x2x2 cube starting from (1, 0, 1)
    set.start_x  = 1;   set.start_y  = 0;    set.start_z  = 1;
    idx.clear();
    idx.push_back(1);   idx.push_back(0);   idx.push_back(1);   // (1, 0, 1) is significant
    idx.push_back(1);   idx.push_back(1);   idx.push_back(1);   // (1, 1, 1) is significant
    decide_subset_significance( set, idx, sigs );
    EXPECT_EQ( sigs[0], speck::Significance::Sig );
    EXPECT_EQ( sigs[1], speck::Significance::Insig );
    EXPECT_EQ( sigs[2], speck::Significance::Sig );
    for( int i = 3; i < 3 + 5; i++ )
        EXPECT_EQ( sigs[i], speck::Significance::Insig );

    // Test 3: a 2x3x2 cube starting from (0, 0, 0)
    set.start_x  = 0;   set.start_y  = 0;    set.start_z  = 0;
    set.length_x = 2;   set.length_y = 3;    set.length_z = 2;
    idx.clear();
    idx.push_back(1);   idx.push_back(2);   idx.push_back(0);   // (1, 2, 0) is significant
    idx.push_back(1);   idx.push_back(1);   idx.push_back(1);   // (1, 1, 1) is significant
    decide_subset_significance( set, idx, sigs );
    EXPECT_EQ( sigs[0], speck::Significance::Insig );
    EXPECT_EQ( sigs[1], speck::Significance::Insig );
    EXPECT_EQ( sigs[2], speck::Significance::Insig );
    EXPECT_EQ( sigs[3], speck::Significance::Sig );
    EXPECT_EQ( sigs[4], speck::Significance::Insig );
    EXPECT_EQ( sigs[5], speck::Significance::Sig );
    EXPECT_EQ( sigs[6], speck::Significance::Insig );
    EXPECT_EQ( sigs[7], speck::Significance::Insig );
}

TEST( speck3d, divide_in_2_or_1_axes )
{
    speck::SPECKSet3D set;
    std::vector< speck::UINT >  idx;
    std::array<speck::Significance, 8> sigs;

    // Test 1: a 2x2x1 cube starting from (0, 0, 0)
    set.start_x  = 0;   set.start_y  = 0;    set.start_z  = 0;
    set.length_x = 2;   set.length_y = 2;    set.length_z = 1;
    idx.push_back(0);   idx.push_back(0);   idx.push_back(0);   // (0, 0, 0) is significant
    idx.push_back(1);   idx.push_back(1);   idx.push_back(0);   // (1, 1, 0) is significant
    decide_subset_significance( set, idx, sigs );
    EXPECT_EQ( sigs[0], speck::Significance::Sig );
    EXPECT_EQ( sigs[1], speck::Significance::Insig );
    EXPECT_EQ( sigs[2], speck::Significance::Insig );
    EXPECT_EQ( sigs[3], speck::Significance::Sig );
    for( int i = 4; i < 4 + 4; i++ )
        EXPECT_EQ( sigs[i], speck::Significance::Insig );

    // Test 2: a 1x2x2 cube starting from (0, 0, 0)
    set.start_x  = 0;   set.start_y  = 0;    set.start_z  = 0;
    set.length_x = 1;   set.length_y = 2;    set.length_z = 2;
    idx.clear();
    idx.push_back(0);   idx.push_back(0);   idx.push_back(0);   // (0, 0, 0) is significant
    idx.push_back(0);   idx.push_back(0);   idx.push_back(1);   // (0, 0, 1) is significant
    decide_subset_significance( set, idx, sigs );
    EXPECT_EQ( sigs[0], speck::Significance::Sig );
    EXPECT_EQ( sigs[1], speck::Significance::Insig );
    EXPECT_EQ( sigs[2], speck::Significance::Insig );
    EXPECT_EQ( sigs[3], speck::Significance::Insig );
    EXPECT_EQ( sigs[4], speck::Significance::Sig );
    for( int i = 5; i < 5 + 3; i++ )
        EXPECT_EQ( sigs[i], speck::Significance::Insig );

    // Test 3: a 1x2x1 cube starting from (0, 0, 0)
    set.start_x  = 0;   set.start_y  = 0;    set.start_z  = 0;
    set.length_x = 1;   set.length_y = 2;    set.length_z = 1;
    idx.clear();
    idx.push_back(0);   idx.push_back(0);   idx.push_back(0);   // (0, 0, 0) is significant
    idx.push_back(0);   idx.push_back(1);   idx.push_back(0);   // (0, 1, 0) is significant
    decide_subset_significance( set, idx, sigs );
    EXPECT_EQ( sigs[0], speck::Significance::Sig );
    EXPECT_EQ( sigs[1], speck::Significance::Insig );
    EXPECT_EQ( sigs[2], speck::Significance::Sig );
    for( int i = 3; i < 3 + 5; i++ )
        EXPECT_EQ( sigs[i], speck::Significance::Insig );

    // Test 4: a 1x3x1 cube starting from (0, 0, 0)
    set.start_x  = 0;   set.start_y  = 0;    set.start_z  = 0;
    set.length_x = 1;   set.length_y = 3;    set.length_z = 1;
    idx.clear();
    idx.push_back(0);   idx.push_back(0);   idx.push_back(0);   // (0, 0, 0) is significant
    idx.push_back(0);   idx.push_back(1);   idx.push_back(0);   // (0, 1, 0) is significant
    decide_subset_significance( set, idx, sigs );
    EXPECT_EQ( sigs[0], speck::Significance::Sig );
    for( int i = 1; i < 1 + 7; i++ )
        EXPECT_EQ( sigs[i], speck::Significance::Insig );
}

}
