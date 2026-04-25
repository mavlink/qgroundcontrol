# ============================================================================
# Qt Multimedia Video Receiver Backend
# Qt6 Multimedia-based video streaming and UVC camera support
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        QtCameraDeviceDiscovery.cc
        QtCameraDeviceDiscovery.h
        QtFfmpegRuntimePolicy.cc
        QtFfmpegRuntimePolicy.h
        QtMultimediaReceiver.cc
        QtMultimediaReceiver.h
        QtPlaybackTrackPolicy.cc
        QtPlaybackTrackPolicy.h
        QtVideoSinkRouter.cc
        QtVideoSinkRouter.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
