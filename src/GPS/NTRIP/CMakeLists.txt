qt_add_library(
    QGCGPSNTRIPSession
    STATIC
    NTRIPError.h
    NTRIPHttpDecoder.cc
    NTRIPHttpDecoder.h
    NTRIPSession.cc
    NTRIPSession.h
    NTRIPStream.cc
    NTRIPStream.h
    NTRIPTransportConfig.cc
    NTRIPTransportConfig.h
    NTRIPRequest.cc
    NTRIPRequest.h
)
target_include_directories(QGCGPSNTRIPSession PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(QGCGPSNTRIPSession PUBLIC cxx_std_20)
target_link_libraries(
    QGCGPSNTRIPSession
    PUBLIC Qt6::Core
    PRIVATE QGCLogging
)
qt_extract_metatypes(QGCGPSNTRIPSession)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSNTRIPSession)

# ============================================================================
# NTRIP Module
# NTRIP client, RTCM parsing, and correction streaming
# ============================================================================

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE NTRIPTlsPolicy.h
            NTRIPConnectionSettings.cc
            NTRIPConnectionStats.cc
            NTRIPConnectionStats.h
            NTRIPGgaProvider.cc
            NTRIPGgaProvider.h
            NTRIPHttpTransport.cc
            NTRIPHttpTransport.h
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

qt_add_qml_module(
    GPSNTRIPModule
    URI QGroundControl.GPS.NTRIP
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES NTRIPConnectionStatus.qml NTRIPConnectionStatusRow.qml NTRIPMountpointList.qml
    NO_PLUGIN
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GPSNTRIPModule)
