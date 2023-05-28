add_library( SPERR
             sperr_helper.cpp
             Bitstream.cpp
             Bitmask.cpp
             Base_Filter.cpp
             Conditioner.cpp
             CDF97.cpp
             SPECK_INT.cpp
             SPECK3D_INT.cpp
             SPECK3D_INT_ENC.cpp
             SPECK3D_INT_DEC.cpp
             SPECK1D_INT.cpp
             SPECK1D_INT_ENC.cpp
             SPECK1D_INT_DEC.cpp
             SPECK_FLT.cpp
             Outlier_Coder.cpp
             SPECK3D_FLT.cpp )
             
target_include_directories( SPERR PUBLIC ${CMAKE_SOURCE_DIR}/include )

if(USE_OMP)
  target_compile_options(   SPERR PUBLIC ${OpenMP_CXX_FLAGS} )
  target_link_libraries(    SPERR PUBLIC OpenMP::OpenMP_CXX )
endif()

#
# Also link zstd
#
if( USE_ZSTD )
    if(USE_BUNDLED_ZSTD)
      # Declare dependency so that `zstd-download` is finished before SPERR is built.
      add_dependencies( SPERR zstd-download )
      target_link_libraries( SPERR PUBLIC zstd )
    else()
      target_link_libraries( SPERR PUBLIC PkgConfig::ZSTD )
    endif()
endif()

#
# LTO
#
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported LANGUAGES CXX)

if( ipo_supported )
    message(STATUS "IPO / LTO enabled")
    set_property(TARGET SPERR PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "IPO / LTO not supported")
endif()

set_target_properties( SPERR PROPERTIES VERSION ${SPERR_VERSION} )

#
# The list of headers is formatted a little cumbersome, but don't change it!
#
set( public_h_list 
"include/sperr_helper.h;\
include/Bitstream.h;\
include/Bitmask.h;\
include/Base_Filter.h;\
include/Conditioner.h;\
include/CDF97.h;\
include/SPECK_INT.h;\
include/SPECK3D_INT.h;\
include/SPECK3D_INT_ENC.h;\
include/SPECK3D_INT_DEC.h;\
include/SPECK1D_INT.h;\
include/SPECK1D_INT_ENC.h;\
include/SPECK1D_INT_DEC.h;\
include/SPECK_FLT.h;\
include/SPECK3D_FLT.h;\
include/Outlier_Coder.h;")
set_target_properties( SPERR PROPERTIES PUBLIC_HEADER "${public_h_list}" )

