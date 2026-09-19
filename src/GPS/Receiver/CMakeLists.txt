if(NOT TARGET QGCGPSReceiverConfig)
    add_library(QGCGPSReceiverConfig STATIC GPSReceiverConfig.cc)
    add_library(QGC::GPSReceiverConfig ALIAS QGCGPSReceiverConfig)
    set_target_properties(
        QGCGPSReceiverConfig
        PROPERTIES AUTOMOC OFF
                   AUTOUIC OFF
                   AUTORCC OFF
                   POSITION_INDEPENDENT_CODE ON
    )
    target_compile_features(QGCGPSReceiverConfig PUBLIC cxx_std_20)
    target_sources(
        QGCGPSReceiverConfig
        PUBLIC FILE_SET
               HEADERS
               FILES
               GPSReceiverConfig.h
               GPSReceiverCapabilities.h
               GPSBaseStationConfig.h
               GPSType.h
    )
endif()

if(NOT TARGET QGCGPSDriverReports)
    add_library(QGCGPSDriverReports INTERFACE)
    add_library(QGC::GPSDriverReports ALIAS QGCGPSDriverReports)
    target_compile_features(QGCGPSDriverReports INTERFACE cxx_std_20)
    target_sources(QGCGPSDriverReports INTERFACE FILE_SET HEADERS FILES GPSDriverReports.h)
endif()
