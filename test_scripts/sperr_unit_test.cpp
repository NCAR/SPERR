#include "SPERR.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <cassert>
#include <random>

namespace {

using speck::RTNType;

class err_tester 
{
private:
    const size_t length = 256 * 128* 415;
    const double tolerance = 0.05;
    
    std::vector<speck::Outlier> LOS, recovered;

public:
    // A method to generate `N` outliers 
    // The resulting outliers will be stored in `LOS`.
    void gen_outliers( size_t N )
    {
        std::random_device rd{};
        std::mt19937 gen{rd()};
        std::normal_distribution<double> val_d{0.0, tolerance};
        std::uniform_int_distribution<size_t> loc_d{0, length-1};

        LOS.clear();
        LOS.reserve(N);
        while( LOS.size() < N ) {
            double val = 0.0;
            while( std::abs(val) <= tolerance )
                val = val_d(gen);

            // Make sure there ain't duplicate locations
            auto loc = loc_d(gen);
            while( std::any_of(LOS.begin(), LOS.end(),
                   [loc](auto& out){return out.location == loc;} )) {
                loc = loc_d(gen);
            }
            LOS.emplace_back(loc, val);
        }
    }

    void test_outliers() {
        // Create an encoder
        speck::SPERR encoder;
        encoder.set_length( length );
        encoder.set_tolerance( tolerance );
        encoder.copy_outlier_list( LOS );

        if( encoder.encode() != RTNType::Good )
            return;
        auto stream = encoder.get_encoded_bitstream();

        // Create a decoder
        speck::SPERR decoder;
        if( decoder.parse_encoded_bitstream(stream.data(), stream.size()) != RTNType::Good )
            return;
        if( decoder.decode() != RTNType::Good )
            return;
        
        recovered = encoder.release_outliers();
    }

    // Now see if every element in LOS is successfully recovered
    bool test_recovery() {
        for( auto& orig : LOS ) {
            if( !std::any_of( recovered.cbegin(), recovered.cend(), 
                [&orig, this](auto& recov) { return (recov.location == orig.location 
                && std::abs(recov.error - orig.error) <= tolerance ); } ) ) {

                printf("failed to recover: (%ld, %f)\n", orig.location, orig.error );
                return false;
            }
        }
        return true;
    }
};


TEST( sperr, small_num_outliers )
{
    err_tester tester;   
    tester.gen_outliers( 100 );
    tester.test_outliers();
    bool success = tester.test_recovery();
    EXPECT_EQ( success, true );
}

TEST( sperr, medium_num_outliers )
{
    err_tester tester;
    tester.gen_outliers( 1000 );
    tester.test_outliers();
    bool success = tester.test_recovery();
    EXPECT_EQ( success, true );
}

TEST( sperr, big_num_outliers )
{
    err_tester tester;
    tester.gen_outliers( 10000 );
    tester.test_outliers();
    bool success = tester.test_recovery();
    EXPECT_EQ( success, true );
}

}
