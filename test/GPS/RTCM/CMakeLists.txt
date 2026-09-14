# RTCM tests.
target_sources(${CMAKE_PROJECT_NAME} PRIVATE RTCMParserTest.cc RTCMParserTest.h RTCMMavlinkTest.cc RTCMMavlinkTest.h
                                             RTCMUdpInputTest.cc RTCMUdpInputTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(RTCMParserTest LABELS Unit)
add_qgc_test(RTCMMavlinkTest LABELS Unit)
add_qgc_test(RTCMUdpInputTest LABELS Unit)
