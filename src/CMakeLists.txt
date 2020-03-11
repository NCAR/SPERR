add_library( speck_helper   speck_helper.cpp  )
add_library( sam_helper     sam_helper.c      )
add_library( CDF97          CDF97.cpp         )
add_library( SPECK2D        SPECK2D.cpp       ) 

target_include_directories( speck_helper INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( sam_helper   INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( CDF97        INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( SPECK2D      INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
