qt_add_library(QGCGPSPassive STATIC GPSDriverPassive.cc GPSDriverPassive.h)
set_target_properties(
    QGCGPSPassive
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
)
target_link_libraries(QGCGPSPassive PUBLIC QGCGPSNativeCommon)
add_library(QGC::GPSPassive ALIAS QGCGPSPassive)
