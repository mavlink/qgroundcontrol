qt_add_library(
    QGCGPSNMEA
    STATIC
    NMEADecoderSession.cc
    NMEADecoderSession.h
    NMEAPositionSource.cc
    NMEAPositionSource.h
    NMEASatelliteAdapter.cc
    NMEASatelliteAdapter.h
    NMEAStreamSplitter.cc
    NMEAStreamSplitter.h
    NMEAUtils.cc
    NMEAUtils.h
)
target_include_directories(QGCGPSNMEA PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    QGCGPSNMEA
    PUBLIC QGCGPSCore
    PRIVATE QGCLogging
)
qt_extract_metatypes(QGCGPSNMEA)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSNMEA)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE NMEASourceManager.cc NMEASourceManager.h)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(${CMAKE_PROJECT_NAME} PRIVATE NMEAConnectionConfig.cc NMEAConnectionConfig.h)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE NMEAConnectionAttempt.cc NMEAConnectionAttempt.h)
