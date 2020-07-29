
#include "SPECK_Err.h"
#include <algorithm>
#include <cassert>
#include <random>
#include "gtest/gtest.h"

namespace {


class err_tester 
{
private:
    const size_t length = 256 * 256 * 256;
    const float  tol    = 0.1f;
    
    std::vector<speck::Outlier> LOS, recovered;

public:
    void test_outliers() {

        // Create an encoder
        speck::SPECK_Err se;
        se.set_length( length );
        se.set_tolerance( tol );

        LOS.clear();
        LOS.emplace_back( 0, 0.15f );
        LOS.emplace_back( 10, -0.15f );
        LOS.emplace_back( length - 1, 0.15f );
        LOS.emplace_back( length / 2, -0.5f );
        LOS.emplace_back( 100, -0.35f );
        se.add_outlier_list( LOS );

        se.encode();
        se.decode();
        
        recovered = se.release_outliers();
    }

    // Now see if every element in LOS is successfully recovered
    bool test_recovery() {
        for( auto& orig : LOS ) {
            if( !std::any_of( recovered.cbegin(), recovered.cend(), 
                [&orig, this](const auto& recov) { return (recov.location == orig.location 
                && std::abs( recov.error - orig.error ) < tol ); } ) ) {

                printf("failed to recover: (%ld, %f)\n", orig.location, orig.error );
                return false;
            }
        }
        return true;
    }
};


TEST( speck_err, manual_outliers )
{
    err_tester tester;   
    tester.test_outliers();
    bool success = tester.test_recovery();
    EXPECT_EQ( success, true );
}

}
