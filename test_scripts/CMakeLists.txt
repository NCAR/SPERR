
add_executable( test_sperr_helper sperr_helper_unit_test.cpp )
target_link_libraries( test_sperr_helper PUBLIC ${SPERR_LIBS}
                                         PUBLIC ${HELPER_LIBS}
                                         PUBLIC gtest_main   )
add_test( NAME test_sperr_helper COMMAND test_sperr_helper   )

add_executable( test_dwt dwt_unit_test.cpp )
target_link_libraries( test_dwt PUBLIC ${CDF_LIBS}
                                PUBLIC ${HELPER_LIBS}
                                PUBLIC gtest_main   )
add_test( NAME test_dwt COMMAND test_dwt            )

add_executable( test_speck2d speck2d_unit_test.cpp )
target_link_libraries( test_speck2d PUBLIC ${SPECK2D_LIBS}
                                    PUBLIC ${SPECK_Storage_LIBS} 
                                    PUBLIC ${CDF_LIBS} 
                                    PUBLIC ${HELPER_LIBS}
                                    PUBLIC gtest_main   )
add_test( NAME test_speck2d COMMAND test_speck2d        )

add_executable( test_speck3d speck3d_unit_test.cpp )
target_link_libraries( test_speck3d PUBLIC ${SPECK3D_LIBS}
                                    PUBLIC ${SPECK_Storage_LIBS} 
                                    PUBLIC ${SPERR_LIBS}
                                    PUBLIC ${CDF_LIBS} 
                                    PUBLIC ${HELPER_LIBS}
                                    PUBLIC gtest_main   )
add_test( NAME test_speck3d COMMAND test_speck3d        )

if( QZ_TERM )
    add_executable( test_sperr sperr_unit_test.cpp )
    target_link_libraries( test_sperr PUBLIC ${SPERR_LIBS}
                                      PUBLIC ${HELPER_LIBS}
                                      PUBLIC gtest_main   )
    add_test( NAME test_sperr COMMAND test_sperr          )
endif()
