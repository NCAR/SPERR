add_executable( show_version show_version.cpp )
target_link_libraries( show_version )

#add_executable( header_parser header_parser.cpp )
#target_link_libraries( header_parser PUBLIC SPERR )

# add_executable( compressor_2d compressor_2d.cpp )
# target_link_libraries( compressor_2d PUBLIC SPERR )

# add_executable( decompressor_2d decompressor_2d.cpp )
# target_link_libraries( decompressor_2d PUBLIC SPERR )

# add_executable( compressor_3d compressor_3d.cpp )
# target_link_libraries( compressor_3d PUBLIC SPERR )

# add_executable( decompressor_3d decompressor_3d.cpp )
# target_link_libraries( decompressor_3d PUBLIC SPERR )

add_executable( sperr3d sperr3d.cpp )
target_link_libraries( sperr3d PUBLIC SPERR PUBLIC CLI11::CLI11 )

add_executable( sperr2d sperr2d.cpp )
target_link_libraries( sperr2d PUBLIC SPERR PUBLIC CLI11::CLI11 )

