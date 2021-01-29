#include "SPERR.h"
#include <algorithm>
#include <cassert>
#include <random>
#include "gtest/gtest.h"

namespace {

using speck::RTNType;

class err_tester 
{
private:
    const size_t length = 256 * 256 * 256;
    const double tol    = 0.1;
    
    std::vector<speck::Outlier> LOS, recovered;

public:
    void test_outliers() {

        // Create an encoder
        speck::SPERR encoder;
        encoder.set_length( length );
        encoder.set_tolerance( tol );

        LOS.clear();
        LOS.emplace_back( 0, 0.15f );
        LOS.emplace_back( 10, -0.15f );
        LOS.emplace_back( length - 1, 0.15f );
        LOS.emplace_back( length / 2, -0.5f );
        LOS.emplace_back( 100, -0.35f );
        encoder.use_outlier_list( LOS );

        if( encoder.encode() != RTNType::Good )
            return;
        auto stream = encoder.get_encoded_bitstream();

        // Create a decoder
        speck::SPERR decoder;
        if( decoder.parse_encoded_bitstream(stream.first.get(), stream.second) != RTNType::Good )
            return;
        if( decoder.decode() != RTNType::Good )
            return;
        
        recovered = encoder.release_outliers();
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


TEST( sperr, manual_outliers )
{
    err_tester tester;   
    tester.test_outliers();
    bool success = tester.test_recovery();
    EXPECT_EQ( success, true );
}

}
