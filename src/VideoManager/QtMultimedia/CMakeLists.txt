# ============================================================================
# Qt Multimedia Video Backend
# Network streams played by QMediaPlayer: the receiver without GStreamer, or by choice (VideoSettings::videoBackend)
# ============================================================================

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        QtMultimediaVideoReceiver.cc
        QtMultimediaVideoReceiver.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
