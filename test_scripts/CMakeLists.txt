enable_testing()

#
# Create a list of libraries that can be linked by apps
#
list( APPEND LINK_LIST SPERR )
list( APPEND LINK_LIST OpenMP::OpenMP_CXX )
if( USE_ZSTD )
    ExternalProject_Get_Property( zstd-download INSTALL_DIR )
    link_directories( "${INSTALL_DIR}/lib" )
    list( APPEND LINK_LIST zstd )
endif()


add_executable( test_sperr_helper sperr_helper_unit_test.cpp )
target_link_libraries( test_sperr_helper ${LINK_LIST} gtest_main )

add_executable( test_dwt dwt_unit_test.cpp )
target_link_libraries( test_dwt ${LINK_LIST} gtest_main )

add_executable( test_speck2d speck2d_unit_test.cpp )
target_link_libraries( test_speck2d ${LINK_LIST} gtest_main )

add_executable( test_speck3d speck3d_unit_test.cpp )
target_link_libraries( test_speck3d ${LINK_LIST} gtest_main )

if( QZ_TERM )
    add_executable( test_sperr sperr_unit_test.cpp )
    target_link_libraries( test_sperr ${LINK_LIST} gtest_main )
endif()

include(GoogleTest)
gtest_discover_tests( test_sperr_helper )
gtest_discover_tests( test_dwt     )
gtest_discover_tests( test_speck2d )
gtest_discover_tests( test_speck3d )
if( QZ_TERM )
  gtest_discover_tests( test_sperr )
endif()
