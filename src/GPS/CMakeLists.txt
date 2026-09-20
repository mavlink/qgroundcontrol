# ============================================================================
# GPS Module
# GPS/GNSS positioning and RTK support
# ============================================================================

# The GPS/RTK core and network correction sources do not require serial support.
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(Core)
add_subdirectory(Transport)
add_subdirectory(Driver/Protocols/NMEA)

add_subdirectory(Driver/Protocols/RTCM)
add_subdirectory(Corrections)
add_subdirectory(Corrections/MAVLink)
add_subdirectory(NTRIP)
add_subdirectory(Positioning)

add_subdirectory(Receiver)
add_subdirectory(Driver)
add_subdirectory(RTK)

# QML registrations belong to the application; the Core library also builds without QML.
target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSManager.cc GPSManager.h GPSQmlTypes.h)

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE Corrections/GPSCorrectionManager.cc
            Corrections/GPSCorrectionManager.h
            Corrections/MAVLink/GPSMavlinkOutput.cc
            Corrections/MAVLink/GPSMavlinkOutput.h
            Corrections/MAVLink/RTCMMavlink.cc
            Corrections/MAVLink/RTCMMavlink.h
            Corrections/RTCMUdpInput.cc
            Corrections/RTCMUdpInput.h
)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSCorrections QGCGPSRTCM QGCGPSMavlinkPacket QGCNetworkIO)
