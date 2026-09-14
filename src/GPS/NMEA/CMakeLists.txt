add_library(QGCGPSNMEAProtocol STATIC NMEASentence.cc NMEAConstellation.cc NMEASatelliteEpoch.cc)
add_library(QGC::GPSNMEAProtocol ALIAS QGCGPSNMEAProtocol)
set_target_properties(
    QGCGPSNMEAProtocol
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
               POSITION_INDEPENDENT_CODE ON
)
target_compile_features(QGCGPSNMEAProtocol PUBLIC cxx_std_20)
target_include_directories(QGCGPSNMEAProtocol PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../Core")
target_sources(QGCGPSNMEAProtocol PUBLIC FILE_SET HEADERS FILES NMEASentence.h NMEAConstellation.h NMEASatelliteEpoch.h)

add_library(QGCGPSNMEA STATIC NMEADecoderSession.cc NMEAPositionSource.cc NMEASatelliteAdapter.cc NMEAStreamSplitter.cc
                              NMEAUtils.cc
)
add_library(QGC::GPSNMEA ALIAS QGCGPSNMEA)
set_target_properties(QGCGPSNMEA PROPERTIES AUTOMOC ON)
target_link_libraries(
    QGCGPSNMEA
    PUBLIC QGCGPSCore QGCGPSNMEAProtocol
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
           NMEAUtils.h
)
if(TARGET ${CMAKE_PROJECT_NAME})
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSNMEA)
endif()
