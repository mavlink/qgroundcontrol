target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSRevisionTest.cc)
add_qgc_test(GPSRevisionTest LABELS Unit GPS)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSNotificationQueueTest.cc)
add_qgc_test(GPSNotificationQueueTest LABELS Unit GPS)
