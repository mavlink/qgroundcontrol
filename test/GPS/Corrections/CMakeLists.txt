add_subdirectory(MAVLink)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSCorrectionRouterTest.cc GPSCorrectionManagerTest.cc
                                             GPSCorrectionManagerTest.h RTCMUdpInputTest.cc RTCMUdpInputTest.h
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(GPSCorrectionRouterTest LABELS Unit GPS)
add_qgc_test(GPSCorrectionManagerTest LABELS Unit RESOURCE_LOCK Settings)
add_qgc_test(RTCMUdpInputTest LABELS Unit)
