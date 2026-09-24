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
              GPSNativePositionReport.h
              GPSNativeSatelliteReport.h
              GPSNativeSurveyReport.h
              GPSProtocolIO.h
              GPSRawAckMatcher.h
              GPSReceiverSettingId.h
)
