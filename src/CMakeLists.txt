add_library( SPERR
             sperr_helper.cpp
             Conditioner.cpp
             CDF97.cpp
             SPECK_Storage.cpp
             SPECK2D.cpp
             SPECK3D.cpp
             SPERR.cpp
             SPECK3D_Compressor.cpp
             SPECK3D_Decompressor.cpp
             SPECK3D_OMP_C.cpp
             SPECK3D_OMP_D.cpp
             SPECK2D_Compressor.cpp
             SPECK2D_Decompressor.cpp )
target_include_directories( SPERR PUBLIC ${CMAKE_SOURCE_DIR}/include )
target_compile_options(     SPERR PUBLIC ${OpenMP_CXX_FLAGS} )
target_link_libraries(      SPERR PUBLIC OpenMP::OpenMP_CXX )
#
# Also link zstd
#
if( USE_ZSTD )
    # Declare dependency so that `zstd-download` is finished before SPERR is built.
    add_dependencies( SPERR zstd-download )
    target_link_libraries( SPERR PUBLIC zstd )
endif()

set_target_properties( SPERR PROPERTIES VERSION ${SPERR_VERSION} )

#
# The list of headers is formatted a little cumbersome, but don't change it!
#
set( public_h_list 
"include/sperr_helper.h;\
include/SPECK3D_Compressor.h;\
include/SPECK3D_Decompressor.h;\
include/SPECK3D_OMP_C.h;\
include/SPECK3D_OMP_D.h;\
include/SPECK2D_Compressor.h;\
include/SPECK2D_Decompressor.h;\
include/Conditioner.h;\
include/CDF97.h;\
include/SPECK_Storage.h;\
include/SPECK2D.h;\
include/SPECK3D.h;\
include/SPERR.h;")
set_target_properties( SPERR PROPERTIES PUBLIC_HEADER "${public_h_list}" )

