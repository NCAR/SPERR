# Tutorial: https://cmake.org/cmake/help/latest/guide/tutorial/index.html

cmake_minimum_required(VERSION 3.10)

project(SPECK VERSION 0.1)

# specify the C++ standard
# CMake will try to add a -std=c++17 flag if the compiler supports, but if it doesn't,
# CMake will `decay` to a previous supported flag, e.g. -std=c++11.
set(CMAKE_CXX_STANDARD 17)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "DEBUG" "RELEASE" "RELWITHDEBINFO")
endif()

option( BUILD_UNIT_TESTS "Build unit tests using googletest?" OFF)
option( USE_ZSTD "Incorporate zstd to achieve further 5% compression?" OFF)
option( USE_PMR  "Use Polymorphic Memory Resource. (Requires a C++17 compiler.)" OFF)
option( QZ_TERM  "Operational mode which terminates by quantization level" ON)
option( USE_OMP  "Enable OpenMP in a few calculations" ON)

if( USE_OMP )
    find_package(OpenMP REQUIRED)
    if( OpenMP_CXX_FOUND AND OpenMP_CXX_FLAGS )
        message(STATUS "OpenMP found! (${OpenMP_CXX_LIB_NAMES})")
    else()
        message(STATUS "OpenMP NOT found!")
    endif()
endif()

#
# configure a header file to pass some of the CMake settings to the source code
#
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/SpeckConfig.h.in" "SpeckConfig.h")
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if( USE_ZSTD )
    project(zstd-download NONE)
    include(ExternalProject)
    ExternalProject_Add( zstd-download
      GIT_REPOSITORY    git@github.com:facebook/zstd.git
      GIT_TAG           master
      GIT_SHALLOW       "True"
      PREFIX            "${CMAKE_CURRENT_BINARY_DIR}/zstd"
      UPDATE_DISCONNECTED "True"                        # Skip update from git step
      SOURCE_SUBDIR     "build/cmake/"                  # Specifies where the CMakeLists.txt file is
      CMAKE_ARGS        "-DCMAKE_BUILD_TYPE=Release"
      CMAKE_ARGS        "-DZSTD_MULTITHREAD_SUPPORT=False"
      CMAKE_ARGS        "-DZSTD_BUILD_PROGRAMS=False"   # Don't build zstd command line utilities
      CMAKE_ARGS        "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/zstd/install"
      INSTALL_DIR       "${CMAKE_CURRENT_BINARY_DIR}/zstd/install"
        # !! <INSTALL_DIR> doesn't contain any value now !!
        # But it can be used later by others to retrieve where the installation is.
      LOG_CONFIGURE     "True"      # Save output in a file, instead of the terminal.
      LOG_BUILD         "True"      # Save output in a file, instead of the terminal.
      LOG_INSTALL       "True"      # Save output in a file, instead of the terminal.
    )
endif()


#
# Add cxxopt
#
#project(cxxopt-download NONE)
#include(ExternalProject)
#ExternalProject_Add( cxxopt-download
#  GIT_REPOSITORY        git@github.com:jarro2783/cxxopts.git
#  GIT_TAG               v2.2.1
#  GIT_SHALLOW           "True"
#  PREFIX                "${CMAKE_CURRENT_BINARY_DIR}/cxxopt"
#  UPDATE_DISCONNECTED   "True"  # Skip update from git step
#  CONFIGURE_COMMAND     ""      # Configure nothing
#  BUILD_COMMAND         ""      # Build nothing
#  INSTALL_COMMAND       ""      # Install nothing
#  LOG_DOWNLOAD          "True"
#)
#ExternalProject_Get_Property( cxxopt-download SOURCE_DIR )
#configure_file("${SOURCE_DIR}/include/cxxopts.hpp" "cxxopts.hpp" COPYONLY)

#
# Download CLI11
#
file(DOWNLOAD https://github.com/CLIUtils/CLI11/releases/download/v1.9.1/CLI11.hpp 
              "${CMAKE_CURRENT_BINARY_DIR}/CLI11.hpp" )

add_subdirectory( src )
list( APPEND HELPER_LIBS speck_helper    )
list( APPEND HELPER_LIBS sam_helper      )

list( APPEND CDF_LIBS CDF97 )

list( APPEND SPECK_Storage_LIBS SPECK_Storage   )

list( APPEND SPECK3D_LIBS SPECK3D_Compressor   )
list( APPEND SPECK3D_LIBS SPECK3D_Decompressor )
list( APPEND SPECK3D_LIBS SPECK3D              )
if( OpenMP_CXX_FOUND )
  list( APPEND SPECK3D_LIBS OpenMP::OpenMP_CXX   )
endif()

list( APPEND SPECK2D_LIBS SPECK2D_Compressor   )
list( APPEND SPECK2D_LIBS SPECK2D_Decompressor )
list( APPEND SPECK2D_LIBS SPECK2D              )

list( APPEND SPECK_Err_LIBS SPECK_Err )

if( USE_ZSTD )
    ExternalProject_Get_Property( zstd-download INSTALL_DIR )
    link_directories( "${INSTALL_DIR}/lib" )
    list( APPEND SPECK_Storage_LIBS zstd )
endif()


add_executable( example_dwt2d "${CMAKE_CURRENT_SOURCE_DIR}/utilities/example_dwt2d.cpp" )
target_link_libraries(      example_dwt2d PUBLIC ${CDF_LIBS}
                                          PUBLIC ${HELPER_LIBS} )
target_include_directories( example_dwt2d PUBLIC ${PROJECT_BINARY_DIR} )

add_executable( example_dwt3d "${CMAKE_CURRENT_SOURCE_DIR}/utilities/example_dwt3d.cpp" )
target_link_libraries(      example_dwt3d PUBLIC ${CDF_LIBS}
                                          PUBLIC ${HELPER_LIBS} )
target_include_directories( example_dwt3d PUBLIC ${PROJECT_BINARY_DIR} )

add_executable( compressor_3d "${CMAKE_CURRENT_SOURCE_DIR}/utilities/compressor_3d.cpp" )
target_link_libraries(      compressor_3d PUBLIC ${SPECK3D_LIBS}
                                          PUBLIC ${SPECK_Storage_LIBS}
                                          PUBLIC ${CDF_LIBS}
                                          PUBLIC ${HELPER_LIBS} )
target_include_directories( compressor_3d PUBLIC ${PROJECT_BINARY_DIR} )

add_executable( compressor_2d "${CMAKE_CURRENT_SOURCE_DIR}/utilities/compressor_2d.cpp" )
target_link_libraries(      compressor_2d PUBLIC ${SPECK2D_LIBS}
                                          PUBLIC ${SPECK_Storage_LIBS}
                                          PUBLIC ${CDF_LIBS}
                                          PUBLIC ${HELPER_LIBS} )
target_include_directories( compressor_2d PUBLIC ${PROJECT_BINARY_DIR} )

add_executable( example_speck_err "${CMAKE_CURRENT_SOURCE_DIR}/utilities/example_speck_err.cpp" )
target_link_libraries(  example_speck_err PUBLIC ${SPECK_Err_LIBS}
                                          PUBLIC ${CDF_LIBS}
                                          PUBLIC ${HELPER_LIBS} )
target_include_directories( example_speck_err PUBLIC ${PROJECT_BINARY_DIR} )


if( BUILD_UNIT_TESTS )
    # Download and unpack googletest at configure time
    configure_file(GTestCMake.txt.in googletest-download/CMakeLists.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .  RESULT_VARIABLE result
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download
                    OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/googletest.stdout
                    ERROR_FILE ${CMAKE_CURRENT_BINARY_DIR}/googletest.stderr        )
    if(result)
      message(FATAL_ERROR "CMake step for googletest failed: ${result}")
    endif()

    execute_process(COMMAND ${CMAKE_COMMAND} --build .  RESULT_VARIABLE result
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download
                    OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/googletest.stdout
                    ERROR_FILE ${CMAKE_CURRENT_BINARY_DIR}/googletest.stderr        )
    if(result)
      message(FATAL_ERROR "Build step for googletest failed: ${result}")
    endif()

    # Prevent overriding the parent project's compiler/linker settings on Windows
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    # Add googletest directly to our build. This defines the gtest and gtest_main targets.
    add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                     ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
                     EXCLUDE_FROM_ALL)

    # Now we have all the tests! 

    add_executable( test_speck_helper 
                    "${CMAKE_CURRENT_SOURCE_DIR}/test_scripts/speck_helper_unit_test.cpp" )
    target_link_libraries( test_speck_helper PUBLIC ${CDF_LIBS}
                                             PUBLIC ${HELPER_LIBS}
                                             PUBLIC gtest_main   )
    add_test( NAME test_speck_helper COMMAND test_speck_helper   )

    add_executable( test_dwt
                    "${CMAKE_CURRENT_SOURCE_DIR}/test_scripts/dwt_unit_test.cpp" )
    target_link_libraries( test_dwt PUBLIC ${CDF_LIBS}
                                    PUBLIC ${SPECK_LIBS} 
                                    PUBLIC ${HELPER_LIBS}
                                    PUBLIC gtest_main   )
    add_test( NAME test_dwt COMMAND test_dwt            )

    add_executable( test_speck2d
                    "${CMAKE_CURRENT_SOURCE_DIR}/test_scripts/speck2d_unit_test.cpp" )
    target_link_libraries( test_speck2d PUBLIC ${SPECK2D_LIBS}
                                        PUBLIC ${SPECK_Storage_LIBS} 
                                        PUBLIC ${CDF_LIBS} 
                                        PUBLIC ${HELPER_LIBS}
                                        PUBLIC gtest_main            )
    add_test( NAME test_speck2d COMMAND test_speck2d                 )

    add_executable( test_speck3d
                    "${CMAKE_CURRENT_SOURCE_DIR}/test_scripts/speck3d_unit_test.cpp" )
    target_link_libraries( test_speck3d PUBLIC ${SPECK3D_LIBS}
                                        PUBLIC ${SPECK_Storage_LIBS} 
                                        PUBLIC ${CDF_LIBS} 
                                        PUBLIC ${HELPER_LIBS}
                                        PUBLIC gtest_main            )
    add_test( NAME test_speck3d COMMAND test_speck3d                 )

    add_executable( test_speck_err
                    "${CMAKE_CURRENT_SOURCE_DIR}/test_scripts/speck_err_unit_test.cpp" )
    target_link_libraries( test_speck_err PUBLIC ${SPECK_Err_LIBS}
                                          PUBLIC ${CDF_LIBS} 
                                        PUBLIC ${HELPER_LIBS}
                                          PUBLIC gtest_main          )
    add_test( NAME test_speck_err COMMAND test_speck_err             )

endif()


#option( BUILD_QCC_EXAMPLES "Build examples that link to QccPack?" OFF)
#[[
if( BUILD_QCC_EXAMPLES )
    set ( QCCPACK_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/QccPack-0.61-1/install/" 
                              CACHE STRING "Where QccPack is installed"             )
    find_library( QCCPACK_LIB QccPack      HINTS ${QCCPACK_INSTALL_DIR}/lib         )
    find_path(    QCCPACK_INC libQccPack.h HINTS ${QCCPACK_INSTALL_DIR}/include     )
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/utilities/CohenDaubechiesFeauveau.9-7.lft" 
         DESTINATION ${PROJECT_BINARY_DIR}                                          )

    # Qcc Executable #1
    add_executable( example_qcc_dwt2d  "${CMAKE_CURRENT_SOURCE_DIR}/utilities/example_qcc_dwt2d.c" )
    target_link_libraries(      example_qcc_dwt2d PUBLIC sam_helper
                                                  PUBLIC ${QCCPACK_LIB}
                                                  PUBLIC m                   )
    target_include_directories( example_qcc_dwt2d PUBLIC ${PROJECT_BINARY_DIR}
                                                  PUBLIC ${QCCPACK_INC}      )

    # Qcc Executable #2
    add_executable( example_qcc_dwt3d "${CMAKE_CURRENT_SOURCE_DIR}/utilities/example_qcc_dwt3d.c" )
    target_link_libraries(      example_qcc_dwt3d PUBLIC sam_helper
                                                  PUBLIC ${QCCPACK_LIB}
                                                  PUBLIC m                   )
    target_include_directories( example_qcc_dwt3d PUBLIC ${PROJECT_BINARY_DIR}
                                                  PUBLIC ${QCCPACK_INC}      )

    # Qcc Executable #3
    add_executable( example_qcc_speck2d "${CMAKE_CURRENT_SOURCE_DIR}/utilities/example_qcc_speck2d.c" )
    target_link_libraries(      example_qcc_speck2d PUBLIC sam_helper
                                                    PUBLIC ${QCCPACK_LIB}
                                                    PUBLIC m                   )
    target_include_directories( example_qcc_speck2d PUBLIC ${PROJECT_BINARY_DIR}
                                                    PUBLIC ${QCCPACK_INC}      )

    # Qcc Executable #4
    add_executable( example_qcc_speck3d "${CMAKE_CURRENT_SOURCE_DIR}/utilities/example_qcc_speck3d.c" )
    target_link_libraries(      example_qcc_speck3d PUBLIC sam_helper
                                                    PUBLIC ${QCCPACK_LIB}
                                                    PUBLIC m                   )
    target_include_directories( example_qcc_speck3d PUBLIC ${PROJECT_BINARY_DIR}
                                                    PUBLIC ${QCCPACK_INC}      )
endif() 
]]
