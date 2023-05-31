add_executable(        test_sperr_helper sperr_helper_unit_test.cpp )
target_link_libraries( test_sperr_helper PUBLIC SPERR gtest_main )

add_executable(        bitstream bitstream_unit_test.cpp )
target_link_libraries( bitstream PUBLIC SPERR gtest_main )

add_executable(        test_dwt dwt_unit_test.cpp )
target_link_libraries( test_dwt PUBLIC SPERR gtest_main )

add_executable(        test_speck_int speck_int_unit_test.cpp )
target_link_libraries( test_speck_int PUBLIC SPERR gtest_main )

add_executable(        test_speck3d speck3d_flt_unit_test.cpp )
target_link_libraries( test_speck3d PUBLIC SPERR gtest_main )

add_executable(        test_outlier_coder outlier_coder_unit_test.cpp )
target_link_libraries( test_outlier_coder PUBLIC SPERR gtest_main )

include(GoogleTest)
gtest_discover_tests( test_sperr_helper )
gtest_discover_tests( bitstream )
gtest_discover_tests( test_dwt     )
gtest_discover_tests( test_speck_int )
gtest_discover_tests( test_speck3d )
gtest_discover_tests( test_outlier_coder )
