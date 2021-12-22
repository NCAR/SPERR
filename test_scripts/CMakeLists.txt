enable_testing()


add_executable( test_sperr_helper sperr_helper_unit_test.cpp )
target_link_libraries( test_sperr_helper SPERR gtest_main )

add_executable( test_dwt dwt_unit_test.cpp )
target_link_libraries( test_dwt SPERR gtest_main )

add_executable( test_speck2d speck2d_unit_test.cpp )
target_link_libraries( test_speck2d SPERR gtest_main )

add_executable( test_speck3d speck3d_unit_test.cpp )
target_link_libraries( test_speck3d SPERR gtest_main )

if( QZ_TERM )
    add_executable( test_sperr sperr_unit_test.cpp )
    target_link_libraries( test_sperr SPERR gtest_main )
endif()

include(GoogleTest)
gtest_discover_tests( test_sperr_helper )
gtest_discover_tests( test_dwt     )
gtest_discover_tests( test_speck2d )
gtest_discover_tests( test_speck3d )
if( QZ_TERM )
  gtest_discover_tests( test_sperr )
endif()
