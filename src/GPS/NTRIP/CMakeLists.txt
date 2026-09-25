# ============================================================================
# NTRIP Module
# NTRIP client, RTCM parsing, and correction streaming
# ============================================================================

if(NOT TARGET QGCNetworkClient)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/Network/Client"
                     "${CMAKE_CURRENT_BINARY_DIR}/NetworkClient"
    )
endif()

qt_add_library(QGCGPSNTRIPHttp STATIC NTRIPConfiguration.cc NTRIPHttpCodec.cc)
add_library(QGC::GPSNTRIPHttp ALIAS QGCGPSNTRIPHttp)
set_target_properties(QGCGPSNTRIPHttp PROPERTIES AUTOMOC ON)
target_compile_features(QGCGPSNTRIPHttp PUBLIC cxx_std_20)
target_link_libraries(
    QGCGPSNTRIPHttp
    PUBLIC Qt6::Core Qt6::Network
    PRIVATE QGCNetworkClient
)
target_sources(QGCGPSNTRIPHttp PUBLIC FILE_SET HEADERS FILES NTRIPConfiguration.h NTRIPError.h NTRIPHttpCodec.h)

if(DEFINED QGC_GPS_COMPONENTS_ENABLED AND NOT "NTRIP" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
    return()
endif()

qt_add_library(
    QGCGPSNTRIP
    STATIC
    NTRIPConnectionStats.cc
    NTRIPGgaProvider.cc
    NTRIPHttpSession.cc
    NTRIPHttpTransport.cc
    NTRIPSourceTableController.cc
    NTRIPSourceTable.cc
    NTRIPTlsPolicy_p.h
)
add_library(QGC::GPSNTRIP ALIAS QGCGPSNTRIP)
set_target_properties(QGCGPSNTRIP PROPERTIES AUTOMOC ON)
target_link_libraries(
    QGCGPSNTRIP
    PUBLIC QGCGPSCore QGCGPSNTRIPHttp QGCGPSRTCM QGCClock QGCIO Qt6::Positioning
    PRIVATE QGCGPSNMEAProtocol QGCLoggingCategory
)
target_sources(
    QGCGPSNTRIP
    PUBLIC FILE_SET
           HEADERS
           FILES
           NTRIPTransport.h
           NTRIPConnectionStats.h
           NTRIPGgaProvider.h
           NTRIPHttpSession.h
           NTRIPHttpTransport.h
           NTRIPSourceTableController.h
           NTRIPSourceTable.h
)

if(NOT TARGET ${CMAKE_PROJECT_NAME})
    return()
endif()

qt_add_library(QGCGPSNTRIPService STATIC NTRIPManager.cc)
add_library(QGC::GPSNTRIPService ALIAS QGCGPSNTRIPService)
set_target_properties(QGCGPSNTRIPService PROPERTIES AUTOMOC ON)
target_link_libraries(
    QGCGPSNTRIPService
    PUBLIC QGCGPSNTRIP QGCGPSCorrections
    PRIVATE QGCGPSCorrectionService QGCLoggingCategory
)
target_sources(QGCGPSNTRIPService PUBLIC FILE_SET HEADERS FILES NTRIPManager.h)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSNTRIPService)
