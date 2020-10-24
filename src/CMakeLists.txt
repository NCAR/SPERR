add_library( speck_helper   speck_helper.cpp  )
add_library( sam_helper     sam_helper.c      )
add_library( CDF97          CDF97.cpp         )
add_library( SPECK_Storage  SPECK_Storage.cpp ) 
add_library( SPECK2D        SPECK2D.cpp       ) 
add_library( SPECK3D        SPECK3D.cpp       ) 
add_library( SPECK_Err      SPECK_Err.cpp     ) 

#
# A user would really want to use the following 4 classes, instead of ones above
#
add_library( SPECK3D_Compressor   SPECK3D_Compressor.cpp   ) 
add_library( SPECK3D_Decompressor SPECK3D_Decompressor.cpp ) 
add_library( SPECK2D_Compressor   SPECK2D_Compressor.cpp   ) 
add_library( SPECK2D_Decompressor SPECK2D_Decompressor.cpp )

target_include_directories( speck_helper  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( sam_helper    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( CDF97         INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( SPECK_Storage INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( SPECK2D       INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( SPECK3D       INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( SPECK_Err     INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )

target_include_directories( SPECK3D_Compressor   INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( SPECK3D_Decompressor INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( SPECK2D_Compressor   INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( SPECK2D_Decompressor INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )

#
# if OpenMP is enabled and found
#
if( USE_OMP AND OpenMP_CXX_FLAGS )
    target_compile_options( SPECK3D PUBLIC ${OpenMP_CXX_FLAGS} )
endif()
    
if( USE_ZSTD )
    add_dependencies( SPECK_Storage zstd-download )
    ExternalProject_Get_Property( zstd-download INSTALL_DIR )
    include_directories("${INSTALL_DIR}/include")
endif()
