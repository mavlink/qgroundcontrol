qt_add_library(QGCGPSNMEAPositioning STATIC NMEADecoderSession.cc NMEAPositionSource.cc NMEASatelliteAdapter.cc
               NMEAStreamSplitter.cc
)
add_library(QGC::GPSNMEAPositioning ALIAS QGCGPSNMEAPositioning)
set_target_properties(QGCGPSNMEAPositioning PROPERTIES AUTOMOC ON)
target_link_libraries(
    QGCGPSNMEAPositioning
    PUBLIC QGCGPSCore QGCGPSNMEAProtocol
    PRIVATE QGCIO QGCLoggingCategory
)
target_sources(
    QGCGPSNMEAPositioning
    PUBLIC FILE_SET
           HEADERS
           FILES
           NMEADecoderSession.h
           NMEAPositionSource.h
           NMEASatelliteAdapter.h
           NMEAStreamSplitter.h
           NMEASentenceEnvelope.h
)
if(TARGET ${CMAKE_PROJECT_NAME})
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE NMEASourceManager.cc NMEASourceManager.h)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSNMEAPositioning)
endif()
