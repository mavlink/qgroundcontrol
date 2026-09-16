if(NOT TARGET QGCGPSNativeContracts)
    add_subdirectory(../Driver/Protocols/Contracts "${CMAKE_CURRENT_BINARY_DIR}/NativeContracts")
endif()

qt_add_library(QGCGPSContracts STATIC GPSReceiverConfig.cc)
add_library(QGC::GPSContracts ALIAS QGCGPSContracts)
set_target_properties(QGCGPSContracts PROPERTIES AUTOMOC ON)
target_compile_features(QGCGPSContracts PUBLIC cxx_std_20)
target_link_libraries(QGCGPSContracts PUBLIC QGC::GPSNativeContracts Qt6::Core)
target_sources(
    QGCGPSContracts
    PUBLIC FILE_SET
           HEADERS
           FILES
           GPSType.h
           GPSReceiverConfig.h
           GPSConnectionError.h
           GPSTransportResult.h
)
