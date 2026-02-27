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

# ----------------------------------------------------------------------------
# Qt Multimedia Video Streaming
# ----------------------------------------------------------------------------
if(QGC_ENABLE_QT_VIDEOSTREAMING)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_QT_STREAMING)
endif()

# ----------------------------------------------------------------------------
# UVC Camera Support
# ----------------------------------------------------------------------------
if(NOT QGC_ENABLE_UVC)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_DISABLE_UVC)
endif()
