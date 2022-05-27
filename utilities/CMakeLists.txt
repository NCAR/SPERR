
add_executable( show_version show_version.cpp )

add_executable( compressor_3d compressor_3d.cpp )
target_link_libraries( compressor_3d PUBLIC SPERR )

add_executable( decompressor_3d decompressor_3d.cpp )
target_link_libraries( decompressor_3d PUBLIC SPERR )

add_executable( compressor_2d compressor_2d.cpp )
target_link_libraries( compressor_2d PUBLIC SPERR )

add_executable( decompressor_2d decompressor_2d.cpp )
target_link_libraries( decompressor_2d PUBLIC SPERR )

add_executable( header_parser header_parser.cpp )
target_link_libraries( header_parser PUBLIC SPERR )

if( QZ_TERM )
  add_executable( probe_3d probe_3d_qz.cpp )
else()
  add_executable( probe_3d probe_3d_size.cpp )
endif()
target_link_libraries( probe_3d PUBLIC SPERR )
