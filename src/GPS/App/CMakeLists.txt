# ============================================================================
# GPS Application Integration
# The only GPS sources compiled into the application: settings, vehicle, and QGC service wiring
# ============================================================================

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE GPSCorrectionStatus.cc
            GPSCorrectionStatus.h
            GPSGgaSources.cc
            GPSGgaSources.h
            GPSManager.cc
            GPSManager.h
            GPSMAVLinkOutput.cc
            GPSMAVLinkOutput.h
            GPSReceiverFactGroup.cc
            GPSReceiverFactGroup.h
            GPSSettingsBindings.cc
            GPSSettingsBindings.h
)
# The GPS receiver core and network correction sources do not require serial support.
if(NOT QGC_NO_SERIAL_LINK)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSSerialPortManagerAdapter.cc GPSSerialPortManagerAdapter.h)
endif()

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core QGCGPSCommon QGCGPSCorrectionService QGCGPSPositioning
                                                    QGCGPSRTCM QGCGPSRTCMMAVLink
)

qt_add_resources(
    ${CMAKE_PROJECT_NAME} json_gps_drivers
    PREFIX "/json/Vehicle"
    FILES GPSReceiverFact.json
)
