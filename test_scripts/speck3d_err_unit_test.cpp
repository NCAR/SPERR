
#include "SPECK3D_Err.h"
#include <algorithm>
#include <cassert>
#include <random>
#include "gtest/gtest.h"

namespace {

//
// Helper function:
//
// The list is already filled with Outliers with all zero's.
// This function assigns x, y, z coordinates using an uniform distribution,
// and assigns err values using a normal distribution that's greater than a tolerance
// 
void assign_outliers( std::vector<speck::Outlier>&  list, 
                      size_t                        dim_x, 
                      size_t                        dim_y,
                      size_t                        dim_z,
                      float                         tolerance )
{
    assert( tolerance > 0.0f );

    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()

    std::uniform_int_distribution<size_t> distrib_x (0, dim_x-1);
    std::uniform_int_distribution<size_t> distrib_y (0, dim_y-1);
    std::uniform_int_distribution<size_t> distrib_z (0, dim_z-1);

    // mean = 0.0, stddev = tolerance
    std::normal_distribution<float> distrib_err (0.0f, tolerance);

    for( auto& e : list ) {
        e.x = distrib_x( gen );
        e.y = distrib_y( gen );
        e.z = distrib_z( gen );
        while( std::abs( e.err ) < tolerance )
            e.err = distrib_err( gen );
    }
}


class err_tester 
{
private:
    const size_t dim_x = 99;
    const size_t dim_y = 121;
    const size_t dim_z = 200;
    const float  tol   = 0.1f;
    const size_t num_of_outs = 1010;
    const size_t num_of_vals = dim_x * dim_y * dim_z;

    std::vector<speck::Outlier> LOS;
    std::vector<speck::Outlier> recovered;

    void fill_normal_distribution() {
        LOS.clear();
        LOS.assign( num_of_outs, speck::Outlier{} );  // default constructor of Outlier
        assign_outliers( LOS, dim_x, dim_y, dim_z, tol );

        // Sort these outliers
        std::sort( LOS.begin(), LOS.end(), [](const auto& a, const auto& b) {
                    if( a.x < b.x )         return true;
                    else if( a.x > b.x )    return false;
                    else if( a.y < b.y )    return true;
                    else if( a.y > b.y )    return false;
                    else if( a.z < b.z )    return true;
                    else if( a.z > b.z )    return false;
                    else                    return false; } );

        // Remove duplicates
        auto it = std::unique( LOS.begin(), LOS.end(), [](const auto& a, const auto& b) {
                        return (a.x == b.x && a.y == b.y && a.z == b.z );
                        } );
        LOS.erase( it, LOS.end() );
    }
    

public:
    void test_normal_distribution() {
        // Create an encoder
        speck::SPECK3D_Err se;
        se.set_dims( dim_x, dim_y, dim_z );
        se.set_tolerance( tol );

        se.add_outlier_list( LOS );

        se.encode();
        se.decode();
        
        recovered = se.release_outliers();
    }

    // Now see if every element in LOS is successfully recovered
    bool test_recovery() {
        for( auto& orig : LOS ) {
            if( !std::any_of( recovered.cbegin(), recovered.cend(), 
                [&orig, this](const auto& recov) { return (recov.x == orig.x && recov.y == orig.y
                && recov.z == orig.z && std::abs( recov.err - orig.err ) < tol ); } ) )
                //printf("failed to recover: (%d, %d, %d, %f)\n", orig.x, orig.y, orig.z, orig.err );
                return false;
        }
        return true;
    }
};


TEST( speck3d_err, normal_distribution )
{
    err_tester tester;   
    tester.test_normal_distribution();
    bool success = tester.test_recovery();
    EXPECT_EQ( success, true );
}

}
