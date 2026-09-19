add_library(QGCGPSNMEA STATIC NMEADecoderSession.cc NMEAPositionSource.cc NMEASatelliteAdapter.cc NMEAStreamSplitter.cc)
add_library(QGC::GPSNMEA ALIAS QGCGPSNMEA)
set_target_properties(QGCGPSNMEA PROPERTIES AUTOMOC ON)
target_link_libraries(
    QGCGPSNMEA
    PUBLIC QGCGPSCore QGCGPSNMEAProtocol QGCGPSNMEAUtils
    PRIVATE QGCIO QGCLoggingCategory
)
target_sources(
    QGCGPSNMEA
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
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSNMEA)
endif()
