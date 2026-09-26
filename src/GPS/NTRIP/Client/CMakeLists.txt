qt_add_library(
    QGCGPSNTRIP
    STATIC
    NTRIPConnectionStats.cc
    NTRIPGgaProvider.cc
    NTRIPHttpTransport.cc
    NTRIPSourceTableController.cc
    NTRIPSourceTable.cc
)
add_library(QGC::GPSNTRIP ALIAS QGCGPSNTRIP)
set_target_properties(QGCGPSNTRIP PROPERTIES AUTOMOC ON)
target_link_libraries(
    QGCGPSNTRIP
    PUBLIC QGCGPSNTRIPHttp
           QGCGPSPositioning
           QGCGPSRTCM
           QGCReentrancy
           QGCClock
           QGCIO
           Qt6::Positioning
    PRIVATE QGCGPSNMEA QGCLoggingCategory
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
