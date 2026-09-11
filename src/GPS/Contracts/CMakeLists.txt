add_subdirectory(../Driver/Protocols/Contracts "${CMAKE_CURRENT_BINARY_DIR}/NativeContracts")
add_library(
    QGCGPSContracts STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSType.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSReceiverFamily.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSReceiverFamily.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSReceiverConfig.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSReceiverConfig.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSReceiverSetting.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSReceiverCapabilities.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSReceiverCapabilities.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSConfigurationReport.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSReceiverProfile.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSReceiverProfile.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSConnectionError.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSTransportResult.h
)
add_library(QGC::GPSContracts ALIAS QGCGPSContracts)
set_target_properties(QGCGPSContracts PROPERTIES AUTOMOC ON)
target_compile_features(QGCGPSContracts PUBLIC cxx_std_20)
target_include_directories(QGCGPSContracts PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(QGCGPSContracts PUBLIC QGCGPSNativeContracts Qt6::Core)
