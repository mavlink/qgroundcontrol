target_sources(${CMAKE_PROJECT_NAME} PRIVATE RTCMMavlinkPacketTest.cc RTCMMavlinkTest.cc RTCMMavlinkTest.h
                                             GPSMavlinkOutputTest.cc GPSMavlinkOutputTest.h
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(RTCMMavlinkPacketTest LABELS Unit GPS)
add_qgc_test(RTCMMavlinkTest LABELS Unit)
add_qgc_test(GPSMavlinkOutputTest LABELS Integration Vehicle)
