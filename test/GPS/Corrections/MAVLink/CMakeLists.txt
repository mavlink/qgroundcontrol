qt_add_executable(QGCRTCMMavlinkPacketTest RTCMMavlinkPacketTest.cc)
set_target_properties(QGCRTCMMavlinkPacketTest PROPERTIES AUTOMOC ON)
target_link_libraries(QGCRTCMMavlinkPacketTest PRIVATE QGC::GPSMavlinkPacket Qt6::Test)
add_test(NAME RTCMMavlinkPacketTest COMMAND QGCRTCMMavlinkPacketTest)
set_tests_properties(RTCMMavlinkPacketTest PROPERTIES LABELS "Unit;GPS" TIMEOUT 60)

if(COMMAND add_qgc_test)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE RTCMMavlinkTest.cc RTCMMavlinkTest.h GPSMavlinkOutputTest.cc
                                                 GPSMavlinkOutputTest.h
    )
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    add_qgc_test(RTCMMavlinkTest LABELS Unit)
    add_qgc_test(GPSMavlinkOutputTest LABELS Integration Vehicle)
endif()
