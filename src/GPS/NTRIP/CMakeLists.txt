# ============================================================================
# NTRIP Module
# NTRIP client, RTCM parsing, and correction streaming
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        NMEAUtils.cc
        NMEAUtils.h
        NTRIPError.h
        NTRIPTransport.h
        NTRIPConnectionStats.cc
        NTRIPConnectionStats.h
        NTRIPGgaProvider.cc
        NTRIPGgaProvider.h
        NTRIPHttpTransport.cc
        NTRIPHttpTransport.h
        NTRIPTransportConfig.cc
        NTRIPTransportConfig.h
        NTRIPManager.cc
        NTRIPManager.h
        NTRIPSourceTableController.cc
        NTRIPSourceTableController.h
        NTRIPSourceTable.cc
        NTRIPSourceTable.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# NTRIP QML Module
# ----------------------------------------------------------------------------
qt_add_library(GPSNTRIPModule STATIC)

qt_add_qml_module(GPSNTRIPModule
    URI QGroundControl.GPS.NTRIP
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        NTRIPConnectionStatus.qml
        NTRIPConnectionStatusRow.qml
        NTRIPMountpointList.qml
    NO_PLUGIN
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GPSNTRIPModule)
