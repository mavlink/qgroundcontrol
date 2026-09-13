if(NOT TARGET QGCGPSNativeContracts)
    add_subdirectory(../Driver/Protocols/Contracts "${CMAKE_CURRENT_BINARY_DIR}/NativeContracts")
endif()
add_library(QGCGPSContracts STATIC ${CMAKE_CURRENT_SOURCE_DIR}/GPSReceiverCapabilities.cc
                                   ${CMAKE_CURRENT_SOURCE_DIR}/GPSReceiverProfile.cc
)
add_library(QGC::GPSContracts ALIAS QGCGPSContracts)
set_target_properties(QGCGPSContracts PROPERTIES AUTOMOC ON)
target_compile_features(QGCGPSContracts PUBLIC cxx_std_20)
target_link_libraries(QGCGPSContracts PUBLIC QGCGPSNativeContracts Qt6::Core)

target_sources(
    QGCGPSContracts
    PUBLIC FILE_SET
           HEADERS
           FILES
           GPSConfigurationReport.h
           GPSConnectionError.h
           GPSReceiverCapabilities.h
           GPSReceiverProfile.h
           GPSReceiverSetting.h
           GPSTransportResult.h
           GPSType.h
)
