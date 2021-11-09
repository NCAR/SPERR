set( utility_output_dir "${CMAKE_BINARY_DIR}/bin" )

add_executable( show_version show_version.cpp )
set_target_properties( show_version PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${utility_output_dir}" )

add_executable( compressor_3d compressor_3d.cpp )
set_target_properties( compressor_3d PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${utility_output_dir}" )
target_link_libraries( compressor_3d PUBLIC ${SPECK3D_LIBS}
                                     PUBLIC ${SPECK_Storage_LIBS}
                                     PUBLIC ${SPERR_LIBS}
                                     PUBLIC ${CDF_LIBS}
                                     PUBLIC ${HELPER_LIBS} )

add_executable( decompressor_3d decompressor_3d.cpp )
set_target_properties( decompressor_3d PROPERTIES RUNTIME_OUTPUT_DIRECTORY  "${utility_output_dir}" )
target_link_libraries( decompressor_3d PUBLIC ${SPECK3D_LIBS}
                                       PUBLIC ${SPECK_Storage_LIBS}
                                       PUBLIC ${SPERR_LIBS}
                                       PUBLIC ${CDF_LIBS}
                                       PUBLIC ${HELPER_LIBS} )

add_executable( compressor_2d compressor_2d.cpp )
set_target_properties( compressor_2d PROPERTIES RUNTIME_OUTPUT_DIRECTORY  "${utility_output_dir}" )
target_link_libraries( compressor_2d PUBLIC ${SPECK2D_LIBS}
                                     PUBLIC ${SPECK_Storage_LIBS}
                                     PUBLIC ${CDF_LIBS}
                                     PUBLIC ${HELPER_LIBS} )

if( QZ_TERM )
  add_executable( probe_3d probe_3d_qz.cpp )
else()
  add_executable( probe_3d probe_3d_size.cpp )
endif()

set_target_properties( probe_3d PROPERTIES RUNTIME_OUTPUT_DIRECTORY  "${utility_output_dir}" )
target_link_libraries( probe_3d PUBLIC ${SPECK3D_LIBS}
                                PUBLIC ${SPECK_Storage_LIBS}
                                PUBLIC ${SPERR_LIBS}
                                PUBLIC ${CDF_LIBS}
                                PUBLIC ${HELPER_LIBS} )
