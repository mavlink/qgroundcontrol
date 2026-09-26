# Developer-only raw stream recording and replay, used by the transport tests and the hardware runner.
qt_add_library(QGCGPSTestTransports STATIC GPSRecordingTransport.cc GPSRecordingTransport.h ReplayGPSTransport.cc
               ReplayGPSTransport.h
)
target_compile_features(QGCGPSTestTransports PUBLIC cxx_std_20)
target_include_directories(QGCGPSTestTransports PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(
    QGCGPSTestTransports
    PUBLIC QGCGPSCommon QGCGPSTransport Qt6::Core
    PRIVATE QGCLoggingCategory
)
