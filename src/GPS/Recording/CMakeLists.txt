add_library(
    QGCGPSRecording STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSRecordingBuffer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSRecordingBuffer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSRecordingFormat.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSRecordingFormat.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSRecordingTransport.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSRecordingTransport.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSRecordingDevice.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSRecordingDevice.h
)
add_library(QGC::GPSRecording ALIAS QGCGPSRecording)
set_target_properties(QGCGPSRecording PROPERTIES AUTOMOC ON)
target_compile_features(QGCGPSRecording PUBLIC cxx_std_20)
target_include_directories(QGCGPSRecording PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    QGCGPSRecording
    PUBLIC QGCGPSContracts QGCGPSTransport QGCGPSCore
    PRIVATE QGCJsonValidation QGCLoggingCategory
)

add_library(QGCGPSRecordingController STATIC ${CMAKE_CURRENT_SOURCE_DIR}/GPSRecordingController.cc
                                             ${CMAKE_CURRENT_SOURCE_DIR}/GPSRecordingController.h
)
add_library(QGC::GPSRecordingController ALIAS QGCGPSRecordingController)
set_target_properties(QGCGPSRecordingController PROPERTIES AUTOMOC ON)
target_compile_features(QGCGPSRecordingController PUBLIC cxx_std_20)
target_include_directories(QGCGPSRecordingController PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    QGCGPSRecordingController
    PUBLIC QGCGPSRecording Qt6::QmlIntegration
    PRIVATE Qt6::Concurrent QGCLoggingCategory
)
