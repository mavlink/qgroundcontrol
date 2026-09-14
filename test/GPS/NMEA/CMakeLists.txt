target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
foreach(gps_test NMEADecoderSessionTest NMEAPositionSourceTest NMEASatelliteAdapterTest NMEAStreamSplitterTest
                 NMEASourceManagerTest NMEASentenceTest
)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${gps_test}.cc ${gps_test}.h)
    if(gps_test STREQUAL "NMEASourceManagerTest")
        add_qgc_test(${gps_test} LABELS Unit RESOURCE_LOCK Settings)
    else()
        add_qgc_test(${gps_test} LABELS Unit)
    endif()
endforeach()
