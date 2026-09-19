# ============================================================================
# GPS Module
# GPS/GNSS positioning and RTK support
# ============================================================================

# The GPS/RTK core and network correction sources do not require serial support.
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(Core)
add_subdirectory(Transport)
add_subdirectory(NMEA)

add_subdirectory(RTCM)
add_subdirectory(Corrections)
add_subdirectory(NTRIP)
add_subdirectory(PositionManager)

add_subdirectory(Receiver)
add_subdirectory(Driver)
add_subdirectory(RTK)

# QML registrations belong to the application; the Core library also builds without QML.
target_sources(${CMAKE_PROJECT_NAME} PRIVATE NMEA/NMEASourceManager.cc NMEA/NMEASourceManager.h GPSManager.cc
                                             GPSManager.h GPSQmlTypes.h
)

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE Corrections/GPSCorrectionManager.cc
            Corrections/GPSCorrectionManager.h
            Corrections/GPSMavlinkOutput.cc
            Corrections/GPSMavlinkOutput.h
            RTCM/RTCMMavlink.cc
            RTCM/RTCMMavlink.h
            RTCM/RTCMUdpInput.cc
            RTCM/RTCMUdpInput.h
)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSCorrections QGCGPSRTCM QGCNetworkIO)
