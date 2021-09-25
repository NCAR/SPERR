#
# Core SPERR libraries
#
add_library( sperr_helper   sperr_helper.cpp  )
add_library( Conditioner    Conditioner.cpp   )
add_library( CDF97          CDF97.cpp         )
add_library( SPECK_Storage  SPECK_Storage.cpp ) 
add_library( SPECK2D        SPECK2D.cpp       ) 
add_library( SPECK3D        SPECK3D.cpp       ) 
add_library( SPERR          SPERR.cpp         ) 

target_include_directories( sperr_helper  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( Conditioner   INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( CDF97         INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( SPECK_Storage INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( SPECK2D       INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( SPECK3D       INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( SPERR         INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )

target_compile_options( sperr_helper  PUBLIC ${OpenMP_CXX_FLAGS} )

#
# Wraps core libraries with easy-to-use interfaces
#
add_library( SPECK3D_Compressor   SPECK3D_Compressor.cpp   ) 
add_library( SPECK3D_Decompressor SPECK3D_Decompressor.cpp ) 
add_library( SPECK3D_OMP_C        SPECK3D_OMP_C.cpp        ) 
add_library( SPECK3D_OMP_D        SPECK3D_OMP_D.cpp        ) 
add_library( SPECK2D_Compressor   SPECK2D_Compressor.cpp   ) 
add_library( SPECK2D_Decompressor SPECK2D_Decompressor.cpp )

target_include_directories( SPECK3D_Compressor   INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( SPECK3D_Decompressor INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( SPECK3D_OMP_C        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( SPECK3D_OMP_D        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( SPECK2D_Compressor   INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( SPECK2D_Decompressor INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )

target_compile_options( SPECK3D_OMP_C PUBLIC ${OpenMP_CXX_FLAGS} )
target_compile_options( SPECK3D_OMP_D PUBLIC ${OpenMP_CXX_FLAGS} )

#
# Though zstd libraries won't be needed until link time,
# those zstd headers are needed at compile time.
# So declare dependency.
#
if( USE_ZSTD )
    add_dependencies( SPECK2D_Compressor   zstd-download )
    add_dependencies( SPECK2D_Decompressor zstd-download )
    add_dependencies( SPECK3D_Compressor   zstd-download )
    add_dependencies( SPECK3D_Decompressor zstd-download )
    add_dependencies( SPECK3D_OMP_C        zstd-download )
    add_dependencies( SPECK3D_OMP_D        zstd-download )
endif()
    

