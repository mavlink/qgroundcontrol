# ============================================================================
# Qt Multimedia Video Receiver Backend
# Qt6 Multimedia-based video streaming and UVC camera support
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        QtMultimediaReceiver.cc
        QtMultimediaReceiver.h
        UVCReceiver.cc
        UVCReceiver.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
