target_sources(${CMAKE_PROJECT_NAME} PRIVATE RTCMMAVLinkPacketTest.cc RTCMMAVLinkTest.cc RTCMMAVLinkTest.h)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(RTCMMAVLinkPacketTest LABELS Unit GPS)
add_qgc_test(RTCMMAVLinkTest LABELS Unit GPS)
