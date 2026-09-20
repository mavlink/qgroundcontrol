qt_add_library(QGCGPSNMEAProtocol STATIC NMEASentence.cc NMEAConstellation.cc NMEASatelliteEpoch.cc NMEAUtils.cc)
add_library(QGC::GPSNMEAProtocol ALIAS QGCGPSNMEAProtocol)
set_target_properties(
    QGCGPSNMEAProtocol
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
               POSITION_INDEPENDENT_CODE ON
)
target_compile_features(QGCGPSNMEAProtocol PUBLIC cxx_std_20)
target_link_libraries(QGCGPSNMEAProtocol PUBLIC Qt6::Core)
target_sources(
    QGCGPSNMEAProtocol
    PUBLIC FILE_SET
           HEADERS
           FILES
           NMEASentence.h
           NMEAConstellation.h
           NMEASatelliteEpoch.h
           NMEAFields.h
           NMEAFramer.h
           NMEAUtils.h
)
target_sources(
    QGCGPSNMEAProtocol
    PUBLIC FILE_SET
           satellite_types
           TYPE
           HEADERS
           BASE_DIRS
           ../../..
           FILES
           ../../../GPSConstellation.h
)
