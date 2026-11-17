# ============================================================================
# Qt Multimedia Video Backend
# Network streams played by QMediaPlayer: the receiver without GStreamer, or by choice (VideoSettings::videoBackend);
# and QtVideoRecorder, the QMediaRecorder that receiver and the USB camera record with
# ============================================================================

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        QtMultimediaVideoReceiver.cc
        QtMultimediaVideoReceiver.h
        QtVideoRecorder.cc
        QtVideoRecorder.h
        SharedVideoFrame.cc
        SharedVideoFrame.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# SharedVideoFrame.cc uses Qt's private video buffer classes, in every build.
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::MultimediaPrivate)
