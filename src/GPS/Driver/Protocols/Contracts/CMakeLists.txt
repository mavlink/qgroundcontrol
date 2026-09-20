if(TARGET QGCGPSNativeContracts)
    return()
endif()
add_library(QGCGPSNativeContracts INTERFACE)
add_library(QGC::GPSNativeContracts ALIAS QGCGPSNativeContracts)
target_compile_features(QGCGPSNativeContracts INTERFACE cxx_std_20)
target_link_libraries(QGCGPSNativeContracts INTERFACE QGCGPSReceiver QGCGPSTransport QGCGPSNMEAProtocol)
target_include_directories(QGCGPSNativeContracts INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(
    QGCGPSNativeContracts
    INTERFACE FILE_SET
              HEADERS
              FILES
              GPSCommandTransaction.h
              GPSDeadline.h
              GPSDecodedBatch.h
              GPSNativeIntegrityReport.h
              GPSNativePositionReport.h
              GPSNativeRelativeReport.h
              GPSNativeSatelliteData.h
              GPSNativeSatelliteReport.h
              GPSNativeSurveyReport.h
              GPSProtocolIO.h
              GPSReceiverSettingId.h
)

foreach(
    family
    UBX
    ASHTECH
    SBF
    FEMTO
    UNICORE
    QUECTEL
    PASSIVE
)
    option(QGC_GPS_ENABLE_${family} "Build the ${family} GPS receiver protocol" ON)
endforeach()
configure_file(GPSProtocolFeatures.h.in GPSProtocolFeatures.h)
target_include_directories(QGCGPSNativeContracts INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
