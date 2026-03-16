# ============================================================================
# Video Manager Module
# Handles video streaming and subtitle generation
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        SubtitleWriter.cc
        SubtitleWriter.h
        VideoManager.cc
        VideoManager.h
        # YoloInference.cc
        # YoloInference.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${OpenCV_LIBS})
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${OpenCV_INCLUDE_DIRS})

# ----------------------------------------------------------------------------
# Video Receiver Subsystem
# ----------------------------------------------------------------------------
add_subdirectory(VideoReceiver)
