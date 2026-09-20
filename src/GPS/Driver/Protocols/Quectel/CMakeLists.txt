qt_add_library(QGCGPSQuectel STATIC GPSDriverQuectel.cc GPSDriverQuectel.h)
set_target_properties(
    QGCGPSQuectel
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
)
target_link_libraries(QGCGPSQuectel PUBLIC QGCGPSNativeCommon)
add_library(QGC::GPSQuectel ALIAS QGCGPSQuectel)
