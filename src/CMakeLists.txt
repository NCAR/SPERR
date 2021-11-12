
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
# Though zstd libraries won't be needed until link time,
# those zstd headers are needed at compile time. So declare dependency.
#
if( USE_ZSTD )
    add_dependencies( SPERR zstd-download )
endif()

