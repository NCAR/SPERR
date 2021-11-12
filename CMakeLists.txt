# Tutorial: https://cmake.org/cmake/help/latest/guide/tutorial/index.html

cmake_minimum_required(VERSION 3.10)

project(SPERR VERSION 0.1 DESCRIPTION "Lossy Scientific Compression with SPERR")

# specify the C++ standard
# CMake will try to add a -std=c++20 flag if the compiler supports, but if it doesn't,
# CMake will `decay` to a previous supported flag, e.g. -std=c++11.
set(CMAKE_CXX_STANDARD 20)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo")
endif()

option( BUILD_UNIT_TESTS "Build unit tests using Google Test" ON)
option( USE_ZSTD "Incorporate ZSTD to achieve further 5% reduction" OFF)
option( QZ_TERM  "Coding terminates by quantization level" ON)

find_package(OpenMP REQUIRED)
if( OpenMP_CXX_FOUND AND OpenMP_CXX_FLAGS )
    message(STATUS "OpenMP found! (${OpenMP_CXX_LIB_NAMES} ${OpenMP_CXX_VERSION}: ${OpenMP_CXX_FLAGS})")
else()
    message(STATUS "OpenMP NOT found!")
endif()

#
# Gather git commit SHA1
#
find_package(Git)
execute_process(COMMAND
  ${GIT_EXECUTABLE} describe --always --dirty
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  OUTPUT_VARIABLE GIT_SHA1
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(COMMAND
  ${GIT_EXECUTABLE} branch --show-current
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  OUTPUT_VARIABLE GIT_BRANCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "Repository Git Branch: ${GIT_BRANCH}, SHA1: ${GIT_SHA1}")

#
# configure a header file to pass some of the CMake settings to the source code
#
configure_file("${CMAKE_SOURCE_DIR}/SperrConfig.h.in" "SperrConfig.h" @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if( QZ_TERM )
    message(STATUS "Compiling in point-wise-error-bounded mode")
else()
    message(STATUS "Compiling in size-bounded mode")
endif()

if( USE_ZSTD )
    message(STATUS "ZSTD add-on enabled!")

    project(zstd-download NONE)
    include(ExternalProject)
    ExternalProject_Add( zstd-download
      GIT_REPOSITORY    git@github.com:facebook/zstd.git
      GIT_TAG           release
      GIT_SHALLOW       "True"
      PREFIX            "${CMAKE_CURRENT_BINARY_DIR}/zstd"
      UPDATE_DISCONNECTED "True"                        # Skip update from git step
      SOURCE_SUBDIR     "build/cmake/"                  # Specifies where the CMakeLists.txt file is
      CMAKE_ARGS        "-DCMAKE_BUILD_TYPE=Release"
      CMAKE_ARGS        "-DZSTD_MULTITHREAD_SUPPORT=False"
      CMAKE_ARGS        "-DZSTD_BUILD_SHARED=False"     # Only build static library
      CMAKE_ARGS        "-DZSTD_BUILD_PROGRAMS=False"   # Don't build zstd command line utilities
      CMAKE_ARGS        "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/zstd/install"
      CMAKE_ARGS        "-DCMAKE_INSTALL_LIBDIR=lib"
      CMAKE_ARGS        "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"         # use the same CXX compiler
      CMAKE_ARGS        "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"             # use the same C compiler
      CMAKE_ARGS        "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}"               # use the same CXX flags
      CMAKE_ARGS        "-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}"                   # use the same C flags
      CMAKE_ARGS        "-DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS}" # use the same linker flags
      INSTALL_DIR       "${CMAKE_CURRENT_BINARY_DIR}/zstd/install"
        # !! <INSTALL_DIR> doesn't contain any value now !!
        # But it can be used later by others to retrieve where the installation is.
      LOG_CONFIGURE     "True"      # Save output in a file, instead of the terminal.
      LOG_BUILD         "True"      # Save output in a file, instead of the terminal.
      LOG_INSTALL       "True"      # Save output in a file, instead of the terminal.
    )

    ExternalProject_Get_Property( zstd-download INSTALL_DIR )
    include_directories("${INSTALL_DIR}/include")
else()
    message(STATUS "ZSTD add-on disabled!")
endif()

#
# Download CLI11
#
if( NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/CLI11.hpp )
    file(DOWNLOAD https://github.com/CLIUtils/CLI11/releases/latest/download/CLI11.hpp
                  ${CMAKE_CURRENT_BINARY_DIR}/CLI11.hpp )
endif()

#
# Compile the SPERR library
#
add_subdirectory( src )

#
# Create a list of libraries that can be liked by apps
#
list( APPEND LINK_LIST SPERR )
list( APPEND LINK_LIST OpenMP::OpenMP_CXX )
if( USE_ZSTD )
    ExternalProject_Get_Property( zstd-download INSTALL_DIR )
    link_directories( "${INSTALL_DIR}/lib" )
    list( APPEND LINK_LIST zstd )
endif()

add_subdirectory( utilities ${CMAKE_BINARY_DIR}/bin )

if( BUILD_UNIT_TESTS )
    enable_testing()

    #
    # Download and unpack googletest at configure time
    #
    configure_file(GTestCMake.txt.in googletest-working/CMakeLists.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .  RESULT_VARIABLE result
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-working
                    OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/googletest-working/googletest.stdout
                    ERROR_FILE ${CMAKE_CURRENT_BINARY_DIR}/googletest-working/googletest.stderr )
    if(result)
      message(FATAL_ERROR "CMake step for googletest failed: ${result}")
    endif()

    execute_process(COMMAND ${CMAKE_COMMAND} --build .  RESULT_VARIABLE result
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-working
                    OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/googletest-working/googletest.stdout
                    ERROR_FILE ${CMAKE_CURRENT_BINARY_DIR}/googletest-working/googletest.stderr )
    if(result)
        message(FATAL_ERROR "Build step for googletest failed: ${result}")
    endif()

    # Prevent overriding the parent project's compiler/linker settings on Windows
    #
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    # Add googletest directly to our build. This defines the gtest and gtest_main targets.
    # These two added directories are essentially the SOURCE_DIR and BINARY_DIR 
    # of the external project (googletest)
    #
    add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest/src/googletest
                     ${CMAKE_CURRENT_BINARY_DIR}/googletest/src/googletest-build
                     EXCLUDE_FROM_ALL)
    #
    # Copy test data sets to the build directory
    #
    file( COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_data DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )

    add_subdirectory( test_scripts )
endif()

# Specify using GNU installation rules
#
include( GNUInstallDirs )
# Install libSPERR.a
#
install( TARGETS SPERR ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                       PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} )
# Install utilities
#
install( TARGETS show_version compressor_3d decompressor_3d probe_3d
         RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )
# Add a pkg-config file
#
configure_file("${CMAKE_SOURCE_DIR}/SPERR.pc.in" SPERR.pc @ONLY)
install( FILES "${CMAKE_BINARY_DIR}/SPERR.pc" DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
# Copy over libzstd.a
#
if( USE_ZSTD )
    ExternalProject_Get_Property( zstd-download INSTALL_DIR )
    install( FILES "${INSTALL_DIR}/lib/libzstd.a" DESTINATION ${CMAKE_INSTALL_LIBDIR} )
endif()
