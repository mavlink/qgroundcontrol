# ============================================================================
# GPS Module
# GPS/GNSS positioning and RTK support
# ============================================================================

# RTCM + NTRIP are network/MAVLink-only — build even when QGC_NO_SERIAL_LINK skips the rest.
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(RTCM)
add_subdirectory(NTRIP)

# Everything below uses QSerialPort or spawns a serial-attached GPS process.
if(QGC_NO_SERIAL_LINK)
    return()
endif()

add_subdirectory(Driver)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        GPSManager.cc
        GPSManager.h
        GPSProvider.cc
        GPSProvider.h
        GPSRtk.cc
        GPSRtk.h
        GPSRTKFactGroup.cc
        GPSRTKFactGroup.h
        SerialGPSTransport.cc
        SerialGPSTransport.h
)

# ----------------------------------------------------------------------------
# GPS/RTK Configuration Resources
# ----------------------------------------------------------------------------
qt_add_resources(${CMAKE_PROJECT_NAME} json_gps_drivers
    PREFIX "/json/Vehicle"
    FILES GPSRTKFact.json
)
