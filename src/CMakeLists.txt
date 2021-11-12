
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
target_compile_options( SPERR PUBLIC ${OpenMP_CXX_FLAGS} )
set_target_properties( SPERR PROPERTIES VERSION ${SPERR_VERSION} )

#
# The list specifying public header is formatted a little cumbersome,
# but don't change it!
#
set( public_h_list 
"include/sperr_helper.h;\
include/SPECK3D_Compressor.h;\
include/SPECK3D_Decompressor.h;\
include/SPECK3D_OMP_C.h;\
include/SPECK3D_OMP_D.h;\
include/SPECK2D_Compressor.h;\
include/SPECK2D_Decompressor.h" )
set_target_properties( SPERR PROPERTIES PUBLIC_HEADER "${public_h_list}" )

#
# Though zstd libraries won't be needed until link time,
# those zstd headers are needed at compile time. So declare dependency.
#
if( USE_ZSTD )
    add_dependencies( SPERR zstd-download )
endif()

