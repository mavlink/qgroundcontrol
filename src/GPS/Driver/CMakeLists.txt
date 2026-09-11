add_subdirectory(Protocols)
add_subdirectory(Transport)
add_library(
    QGCGPSDriver STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSDriver.cc ${CMAKE_CURRENT_SOURCE_DIR}/GPSDriver.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSDriverBackend.cc ${CMAKE_CURRENT_SOURCE_DIR}/GPSDriverBackend.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSDriverData.cc ${CMAKE_CURRENT_SOURCE_DIR}/GPSDriverData.h
)
add_library(QGC::GPSDriver ALIAS QGCGPSDriver)
set_target_properties(QGCGPSDriver PROPERTIES AUTOMOC ON)
target_compile_features(QGCGPSDriver PUBLIC cxx_std_20)
target_include_directories(QGCGPSDriver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    QGCGPSDriver
    PUBLIC QGCGPSContracts QGCGPSCore QGCGPSTransport QGCGPSNativeContracts
    PRIVATE QGCGPSNative QGCLoggingCategory
)
