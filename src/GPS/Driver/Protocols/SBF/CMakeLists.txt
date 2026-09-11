add_library(QGCGPSSBF STATIC GPSDriverSBF.cc GPSDriverSBF.h SBFConfiguration.cc SBFDecoder.cc SBFMessages.h
                             SBFPrivate.h
)
set_target_properties(
    QGCGPSSBF
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
)
target_link_libraries(QGCGPSSBF PUBLIC QGCGPSNativeCommon)
add_library(QGC::GPSSBF ALIAS QGCGPSSBF)
