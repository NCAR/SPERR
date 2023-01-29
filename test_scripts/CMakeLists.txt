add_executable(        sperr_helper sperr_helper_unit_test.cpp )
target_link_libraries( sperr_helper PUBLIC SPERR gtest_main )

add_executable(        dwt dwt_unit_test.cpp )
target_link_libraries( dwt PUBLIC SPERR gtest_main )

add_executable(        sperr2d sperr2d_unit_test.cpp )
target_link_libraries( sperr2d PUBLIC SPERR gtest_main )

add_executable(        sperr3d sperr3d_unit_test.cpp )
target_link_libraries( sperr3d PUBLIC SPERR gtest_main )

add_executable(        sperr sperr_unit_test.cpp )
target_link_libraries( sperr PUBLIC SPERR gtest_main )

include(GoogleTest)
gtest_discover_tests( sperr_helper )
gtest_discover_tests( dwt     )
gtest_discover_tests( sperr2d )
gtest_discover_tests( sperr3d )
gtest_discover_tests( sperr )
