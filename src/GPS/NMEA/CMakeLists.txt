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
target_sources(QGCGPSNMEAProtocol PUBLIC FILE_SET HEADERS FILES NMEASentence.h NMEAConstellation.h NMEASatelliteEpoch.h)
target_sources(
    QGCGPSNMEAProtocol
    PUBLIC FILE_SET
           satellite_types
           TYPE
           HEADERS
           BASE_DIRS
           ..
           FILES
           ../GPSConstellation.h
)

if(DEFINED QGC_GPS_COMPONENTS_ENABLED
   AND NOT "NMEA" IN_LIST QGC_GPS_COMPONENTS_ENABLED
   AND NOT "NMEAUtils" IN_LIST QGC_GPS_COMPONENTS_ENABLED
)
    return()
endif()

add_library(QGCGPSNMEAUtils STATIC NMEAUtils.cc)
add_library(QGC::GPSNMEAUtils ALIAS QGCGPSNMEAUtils)
target_compile_features(QGCGPSNMEAUtils PUBLIC cxx_std_20)
target_link_libraries(
    QGCGPSNMEAUtils
    PUBLIC Qt6::Core Qt6::Positioning
    PRIVATE QGCGPSNMEAProtocol
)
target_sources(QGCGPSNMEAUtils PUBLIC FILE_SET HEADERS FILES NMEAUtils.h)

if(DEFINED QGC_GPS_COMPONENTS_ENABLED AND NOT "NMEA" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
    return()
endif()

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
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSNMEA)
endif()
