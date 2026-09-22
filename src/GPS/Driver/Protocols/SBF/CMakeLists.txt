qt_add_library(QGCGPSSBF STATIC GPSDriverSBF.h SBFConfiguration.cc SBFDecoder.cc SBFMessages.h)
set_target_properties(
    QGCGPSSBF
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
)
target_link_libraries(QGCGPSSBF PUBLIC QGCGPSNativeCommon)
add_library(QGC::GPSSBF ALIAS QGCGPSSBF)
