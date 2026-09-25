if(TARGET QGCGPSProtocolContracts)
    return()
endif()
add_library(QGCGPSProtocolContracts INTERFACE)
add_library(QGC::GPSProtocolContracts ALIAS QGCGPSProtocolContracts)
target_compile_features(QGCGPSProtocolContracts INTERFACE cxx_std_20)
target_link_libraries(QGCGPSProtocolContracts INTERFACE QGCGPSReceiver QGCGPSTransport QGCGPSNMEAProtocol)
target_include_directories(QGCGPSProtocolContracts INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(
    QGCGPSProtocolContracts
    INTERFACE FILE_SET
              HEADERS
              FILES
              GPSCommandTransaction.h
              GPSDeadline.h
              GPSDecodedBatch.h
              GPSProtocolIO.h
              GPSRawAckMatcher.h
              GPSReceiverSettingId.h
)
