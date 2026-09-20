qt_add_library(QGCGPSUnicore STATIC GPSDriverUnicore.cc GPSDriverUnicore.h)
set_target_properties(
    QGCGPSUnicore
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
)
target_link_libraries(QGCGPSUnicore PUBLIC QGCGPSNativeCommon)
add_library(QGC::GPSUnicore ALIAS QGCGPSUnicore)
