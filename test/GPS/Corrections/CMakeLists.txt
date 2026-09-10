# RTCM tests.
target_sources(${CMAKE_PROJECT_NAME} PRIVATE RTCMParserTest.cc RTCMParserTest.h RTCMMavlinkTest.cc RTCMMavlinkTest.h
                                             RTCMUdpInputTest.cc RTCMUdpInputTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(RTCMParserTest LABELS Unit)
add_qgc_test(RTCMMavlinkTest LABELS Unit)
add_qgc_test(RTCMUdpInputTest LABELS Unit)

qt_add_executable(QGCRTCMConformanceTest RTCMConformanceTest.cc)
target_include_directories(QGCRTCMConformanceTest PRIVATE ../../../src/GPS/Corrections
                                                          ../../../src/GPS/Driver/Protocols
)
target_link_libraries(QGCRTCMConformanceTest PRIVATE Qt6::Test QGCLogging QGCGPSCorrections)
add_test(NAME RTCMConformanceTest COMMAND QGCRTCMConformanceTest)
set_tests_properties(RTCMConformanceTest PROPERTIES LABELS "Unit;GPS" TIMEOUT 60)

qt_add_executable(QGCGPSCorrectionRouterTest GPSCorrectionRouterTest.cc)
target_link_libraries(QGCGPSCorrectionRouterTest PRIVATE QGCGPSCorrections Qt6::Test)
add_test(NAME GPSCorrectionRouterTest COMMAND QGCGPSCorrectionRouterTest)
set_tests_properties(GPSCorrectionRouterTest PROPERTIES LABELS "Unit;GPS" TIMEOUT 60)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSCorrectionManagerTest.cc GPSCorrectionManagerTest.h)
add_qgc_test(GPSCorrectionManagerTest LABELS Unit RESOURCE_LOCK Settings)
