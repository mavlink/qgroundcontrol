qt_add_library(
    QGCGPSRecording
    STATIC
    GPSRecordingBuffer.cc
    GPSRecordingBuffer.h
    GPSRecordingTransport.cc
    GPSRecordingTransport.h
    GPSRecordingDevice.cc
    GPSRecordingDevice.h
)
target_include_directories(QGCGPSRecording PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    QGCGPSRecording
    PUBLIC QGCGPSDriver Qt6::Core
    PRIVATE QGCLogging
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSRecordingController.cc GPSRecordingController.h)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSRecording)
