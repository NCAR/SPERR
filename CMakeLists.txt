# Tutorial: https://cmake.org/cmake/help/latest/guide/tutorial/index.html

cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(Speck VERSION 0.1)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option( EIGHT_BYTE_SPECK_SET "Use eight bytes for speck sets?" OFF)
option( PRINT "Compile print code for debug purpose?" OFF)

# configure a header file to pass some of the CMake settings to the source code
configure_file(SpeckConfig.h.in SpeckConfig.h)

add_subdirectory( src )
list( APPEND SPECK_LIBS speck_helper )
list( APPEND SPECK_LIBS sam_helper )
list( APPEND SPECK_LIBS CDF97 )
list( APPEND SPECK_LIBS SPECK2D )

add_executable( test_sam_dwt2d utilities/test_sam_dwt2d.cpp )

target_link_libraries( test_sam_dwt2d PUBLIC ${SPECK_LIBS})

target_include_directories( test_sam_dwt2d PUBLIC
                            ${PROJECT_BINARY_DIR}
                          )
