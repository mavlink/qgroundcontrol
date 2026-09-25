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
add_subdirectory(Corrections/MAVLink)
add_subdirectory(NTRIP)
add_subdirectory(Positioning)

add_subdirectory(Receiver)
add_subdirectory(Driver)
add_subdirectory(RTK)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSManager.cc GPSManager.h)

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

# ----------------------------------------------------------------------------
# GPS QML Module
# QML types of the GPS libraries, which build without QML, and shared GPS components.
# ----------------------------------------------------------------------------
qt_add_library(GPSModule STATIC)

set_source_files_properties(GPSFormat.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(
    GPSModule
    URI QGroundControl.GPS
    VERSION 1.0
    RESOURCE_PREFIX /qml
    SOURCES GPSQmlTypes.h
    QML_FILES GPSFormat.qml NTRIP/NTRIPConnectionStatus.qml NTRIP/NTRIPMountpointList.qml NTRIP/RTCMMessageChips.qml
)

target_link_libraries(
    GPSModule
    PRIVATE Qt6::Qml
            QGCGPSCore
            QGCGPSCorrections
            QGCGPSNTRIP
            QGCGPSPositioning
            QGCGPSReceiver
            QGCGPSRTCM
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GPSModule)
