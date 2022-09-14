add_library( SPERR
             sperr_helper.cpp
             Conditioner.cpp
             CDF97.cpp
             SPECK_Storage.cpp
             SPECK2D.cpp
             SPECK3D.cpp
             SPERR.cpp
             SPERR3D_Compressor.cpp
             SPERR3D_Decompressor.cpp
             SPERR3D_OMP_C.cpp
             SPERR3D_OMP_D.cpp
             SPERR2D_Compressor.cpp
             SPERR2D_Decompressor.cpp
             SPERR_C_API.cpp)
target_include_directories( SPERR PUBLIC ${CMAKE_SOURCE_DIR}/include )

if(USE_OMP)
  target_compile_options(   SPERR PUBLIC ${OpenMP_CXX_FLAGS} )
  target_link_libraries(    SPERR PUBLIC OpenMP::OpenMP_CXX )
endif()

#
# Also link zstd
#
if( USE_ZSTD )
    # Declare dependency so that `zstd-download` is finished before SPERR is built.
    add_dependencies( SPERR zstd-download )
    target_link_libraries( SPERR PUBLIC zstd )
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
include/SPERR3D_Compressor.h;\
include/SPERR3D_Decompressor.h;\
include/SPERR3D_OMP_C.h;\
include/SPERR3D_OMP_D.h;\
include/SPERR2D_Compressor.h;\
include/SPERR2D_Decompressor.h;\
include/Conditioner.h;\
include/CDF97.h;\
include/SPECK_Storage.h;\
include/SPECK2D.h;\
include/SPECK3D.h;\
include/SPERR_C_API.h;\
include/SPERR.h;")
set_target_properties( SPERR PROPERTIES PUBLIC_HEADER "${public_h_list}" )

