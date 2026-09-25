qt_add_library(QGCGPSNTRIPService STATIC NTRIPManager.cc NTRIPStreamSession.cc)
add_library(QGC::GPSNTRIPService ALIAS QGCGPSNTRIPService)
set_target_properties(QGCGPSNTRIPService PROPERTIES AUTOMOC ON)
target_link_libraries(
    QGCGPSNTRIPService
    PUBLIC QGCGPSNTRIP QGCGPSCorrections
    PRIVATE QGCGPSCorrectionService QGCLoggingCategory Qt6::Network
)
target_sources(QGCGPSNTRIPService PRIVATE NTRIPNetworkMonitor.cc)
target_sources(QGCGPSNTRIPService PUBLIC FILE_SET HEADERS FILES NTRIPManager.h NTRIPNetworkMonitor.h
                                         NTRIPStreamSession.h
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSNTRIPService)
