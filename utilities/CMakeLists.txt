#
# Good documentation on CMake's handle or rpath:
#   https://gitlab.kitware.com/cmake/community/-/wikis/doc/cmake/RPATH-handling
# and a good discussion on rpath topics:
#   https://stackoverflow.com/questions/32469953/why-is-cmake-designed-so-that-it-removes-runtime-path-when-installing
#
set( CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_LIBDIR} )
set( CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE )

add_executable( show_version show_version.cpp )

add_executable( compressor_3d compressor_3d.cpp )
target_link_libraries( compressor_3d PUBLIC SPERR )

add_executable( decompressor_3d decompressor_3d.cpp )
target_link_libraries( decompressor_3d PUBLIC SPERR )

add_executable( compressor_2d compressor_2d.cpp )
target_link_libraries( compressor_2d PUBLIC SPERR )

if( QZ_TERM )
  add_executable( probe_3d probe_3d_qz.cpp )
else()
  add_executable( probe_3d probe_3d_size.cpp )
endif()
target_link_libraries( probe_3d PUBLIC SPERR )
