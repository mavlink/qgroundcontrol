# RTCM tests.
target_sources(${CMAKE_PROJECT_NAME} PRIVATE RTCMParserTest.cc RTCMParserTest.h RTCMMavlinkTest.cc RTCMMavlinkTest.h
                                             RTCMUdpInputTest.cc RTCMUdpInputTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(RTCMParserTest LABELS Unit)
add_qgc_test(RTCMMavlinkTest LABELS Unit)
add_qgc_test(RTCMUdpInputTest LABELS Unit)

qt_add_executable(QGCRTCMConformanceTest RTCMConformanceTest.cc ../../../src/GPS/RTCM/RTCMParser.cc
                  ../../../src/GPS/Driver/PX4/rtcm.cpp
)
target_include_directories(QGCRTCMConformanceTest PRIVATE ../../../src/GPS/RTCM ../../../src/GPS/Driver/PX4)
target_link_libraries(QGCRTCMConformanceTest PRIVATE Qt6::Test QGCLogging)
add_test(NAME RTCMConformanceTest COMMAND QGCRTCMConformanceTest)
set_tests_properties(RTCMConformanceTest PROPERTIES LABELS "Unit;GPS" TIMEOUT 60)
