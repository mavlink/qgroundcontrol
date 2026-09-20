# ============================================================================
# NTRIP Module
# NTRIP client, RTCM parsing, and correction streaming
# ============================================================================

if(NOT TARGET QGCNetworkClient)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/Network/Client"
                     "${CMAKE_CURRENT_BINARY_DIR}/NetworkClient"
    )
endif()

qt_add_library(QGCGPSNTRIPHttp STATIC NTRIPConfiguration.cc NTRIPHttpDecoder.cc NTRIPHttpRequest.cc)
add_library(QGC::GPSNTRIPHttp ALIAS QGCGPSNTRIPHttp)
set_target_properties(QGCGPSNTRIPHttp PROPERTIES AUTOMOC ON)
target_compile_features(QGCGPSNTRIPHttp PUBLIC cxx_std_20)
target_link_libraries(
    QGCGPSNTRIPHttp
    PUBLIC Qt6::Core Qt6::Network
    PRIVATE QGCNetworkClient
)
target_sources(
    QGCGPSNTRIPHttp
    PUBLIC FILE_SET
           HEADERS
           FILES
           NTRIPConfiguration.h
           NTRIPError.h
           NTRIPHttpDecoder.h
           NTRIPHttpRequest.h
)

if(DEFINED QGC_GPS_COMPONENTS_ENABLED AND NOT "NTRIP" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
    return()
endif()

qt_add_library(
    QGCGPSNTRIP
    STATIC
    NTRIPConnectionStats.cc
    NTRIPGgaProvider.cc
    NTRIPHttpTransport.cc
    NTRIPSourceTableController.cc
    NTRIPSourceTable.cc
    NTRIPTlsPolicy_p.h
)
add_library(QGC::GPSNTRIP ALIAS QGCGPSNTRIP)
set_target_properties(QGCGPSNTRIP PROPERTIES AUTOMOC ON)
target_link_libraries(
    QGCGPSNTRIP
    PUBLIC QGCGPSNTRIPHttp QGCGPSRTCM QGCClock QGCIO Qt6::Positioning
    PRIVATE QGCGPSNMEAProtocol QGCNetworkClient QGCLoggingCategory
)
target_sources(
    QGCGPSNTRIP
    PUBLIC FILE_SET
           HEADERS
           FILES
           NTRIPTransport.h
           NTRIPConnectionStats.h
           NTRIPGgaProvider.h
           NTRIPHttpTransport.h
           NTRIPSourceTableController.h
           NTRIPSourceTable.h
)

if(NOT TARGET ${CMAKE_PROJECT_NAME})
    return()
endif()

target_sources(${CMAKE_PROJECT_NAME} PRIVATE NTRIPManager.cc NTRIPManager.h)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSNTRIP)

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
