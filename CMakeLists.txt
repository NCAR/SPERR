# Tutorial: https://cmake.org/cmake/help/latest/guide/tutorial/index.html

cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(Speck VERSION 0.1)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

option( EIGHT_BYTE_SPECK_SET "Use eight bytes for speck sets?" OFF)
option( PRINT "Compile print statements for debug purpose?" OFF)
option( TIME  "Compile timing code for debug purpose?" OFF)
option( BUILD_QCC_TESTS "Build test utilities that link to QccPack?" OFF)

# configure a header file to pass some of the CMake settings to the source code
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/SpeckConfig.h.in" "SpeckConfig.h")

add_subdirectory( src )
list( APPEND SPECK_LIBS speck_helper )
list( APPEND SPECK_LIBS sam_helper )
list( APPEND SPECK_LIBS CDF97 )
list( APPEND SPECK_LIBS SPECK2D )

# Executable #1
add_executable(             test_sam_dwt2d "${CMAKE_CURRENT_SOURCE_DIR}/utilities/test_sam_dwt2d.cpp" )
target_link_libraries(      test_sam_dwt2d PUBLIC ${SPECK_LIBS})
target_include_directories( test_sam_dwt2d PUBLIC ${PROJECT_BINARY_DIR} )

# Executable #2
add_executable(             test_sam_speck2d "${CMAKE_CURRENT_SOURCE_DIR}/utilities/test_sam_speck2d.cpp" )
target_link_libraries(      test_sam_speck2d PUBLIC ${SPECK_LIBS})
target_include_directories( test_sam_speck2d PUBLIC ${PROJECT_BINARY_DIR} )

if( BUILD_QCC_TESTS )
    set ( QCCPACK_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/QccPack-0.61-1/install/" 
                              CACHE STRING "Where QccPack is installed"             )
    find_library( QCCPACK_LIB QccPack      HINTS ${QCCPACK_INSTALL_DIR}/lib         )
    find_path(    QCCPACK_INC libQccPack.h HINTS ${QCCPACK_INSTALL_DIR}/include     )
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/utilities/CohenDaubechiesFeauveau.9-7.lft" 
         DESTINATION ${PROJECT_BINARY_DIR}                                          )

    # Qcc Executable #1
    add_executable(             test_qcc_dwt2d  "${CMAKE_CURRENT_SOURCE_DIR}/utilities/test_qcc_dwt2d.c" )
    target_link_libraries(      test_qcc_dwt2d  PUBLIC sam_helper
                                                PUBLIC ${QCCPACK_LIB}      )
    target_include_directories( test_qcc_dwt2d  PUBLIC ${PROJECT_BINARY_DIR}
                                                PUBLIC ${QCCPACK_INC}      )

    # Qcc Executable #2
    add_executable(             test_qcc_speck2d  "${CMAKE_CURRENT_SOURCE_DIR}/utilities/test_qcc_speck2d.c" )
    target_link_libraries(      test_qcc_speck2d  PUBLIC sam_helper
                                                  PUBLIC ${QCCPACK_LIB}      )
    target_include_directories( test_qcc_speck2d  PUBLIC ${PROJECT_BINARY_DIR}
                                                  PUBLIC ${QCCPACK_INC}      )
endif()
