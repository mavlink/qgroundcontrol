qt_add_library(
    QGCGPSNMEA
    STATIC
    NMEASentence.cc
    NMEAConstellation.cc
    NMEASatelliteEpoch.cc
    NMEANavigationEpoch.cc
    NMEAUtils.cc
)
add_library(QGC::GPSNMEA ALIAS QGCGPSNMEA)
set_target_properties(
    QGCGPSNMEA
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
               POSITION_INDEPENDENT_CODE ON
)
target_compile_features(QGCGPSNMEA PUBLIC cxx_std_20)
target_link_libraries(QGCGPSNMEA PUBLIC Qt6::Core QGCGPSCommon)
target_include_directories(QGCGPSNMEA PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/Math")
target_sources(
    QGCGPSNMEA
    PUBLIC FILE_SET
           HEADERS
           FILES
           NMEASentence.h
           NMEAConstellation.h
           NMEASatelliteEpoch.h
           NMEAFields.h
           NMEAFramer.h
           NMEALineFramer.h
           NMEAMetadata.h
           NMEANavigationEpoch.h
           NMEAUtils.h
)
