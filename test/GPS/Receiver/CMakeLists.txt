target_sources(${CMAKE_PROJECT_NAME}
               PRIVATE GPSProviderTest.cc GPSProviderTest.h GPSReceiverPositionSourceTest.cc
                       GPSReceiverPositionSourceTest.h GPSReceiverSessionTest.cc GPSReceiverSessionTest.h
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_qgc_test(GPSProviderTest LABELS Unit)
add_qgc_test(GPSReceiverPositionSourceTest LABELS Unit)
add_qgc_test(GPSReceiverSessionTest LABELS Unit)
