qt_add_library(
    QGCGPSReceiver
    STATIC
    GPSByteStream.cc
    GPSByteStream.h
    GPSProvider.cc
    GPSProvider.h
    GPSReceiverPositionSource.cc
    GPSReceiverPositionSource.h
    GPSReceiverSession.cc
    GPSReceiverSession.h
)

target_include_directories(QGCGPSReceiver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    QGCGPSReceiver
    PUBLIC QGCGPSDriver QGCGPSCore
    PRIVATE QGCLogging
)
