target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSReceiverConfigTest.cc)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSReceiver)

add_qgc_test(GPSReceiverConfigTest LABELS Unit GPS)
