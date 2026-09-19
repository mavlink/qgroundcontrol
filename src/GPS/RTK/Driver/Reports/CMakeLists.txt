if(TARGET QGCGPSDriverData)
    return()
endif()

add_library(QGCGPSDriverData STATIC ../GPSDriverData.cc)
add_library(QGC::GPSDriverData ALIAS QGCGPSDriverData)
target_compile_features(QGCGPSDriverData PUBLIC cxx_std_20)
set_target_properties(
    QGCGPSDriverData
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
)
target_sources(QGCGPSDriverData PUBLIC FILE_SET HEADERS FILES GPSDriverReports.h)
target_include_directories(QGCGPSDriverData PRIVATE .. ../..)
