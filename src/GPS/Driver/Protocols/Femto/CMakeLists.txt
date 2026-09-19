add_library(QGCGPSFemto STATIC FemtoConfiguration.cc FemtoDecoder.cc FemtoMessages.h FemtoPrivate.h GPSDriverFemto.cc
                               GPSDriverFemto.h
)
set_target_properties(
    QGCGPSFemto
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
)
target_link_libraries(QGCGPSFemto PUBLIC QGCGPSNativeCommon)
add_library(QGC::GPSFemto ALIAS QGCGPSFemto)
