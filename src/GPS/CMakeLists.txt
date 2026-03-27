# ============================================================================
# GPS Module
# GPS/GNSS positioning, RTK support, and NTRIP corrections
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        GPSEvent.h
        GPSEventModel.cc
        GPSEventModel.h
        GPSManager.cc
        GPSManager.h
        SatelliteModel.cc
        SatelliteModel.h
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# GPS QML Module
# ----------------------------------------------------------------------------
qt_add_library(GPSModule STATIC)

target_sources(GPSModule
    PRIVATE
        GPSFormatter.cc
        GPSFormatter.h
)

target_link_libraries(GPSModule PRIVATE Qt6::Core Qt6::QmlIntegration)

qt_add_qml_module(GPSModule
    URI QGroundControl.GPS
    VERSION 1.0
    RESOURCE_PREFIX /qml
)

# ----------------------------------------------------------------------------
# Subsystems
# ----------------------------------------------------------------------------
add_subdirectory(Driver)
add_subdirectory(RTK)
add_subdirectory(NTRIP)
