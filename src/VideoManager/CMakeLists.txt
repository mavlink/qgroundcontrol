# ============================================================================
# Video Manager Module
# Handles video streaming, recording and subtitle generation
# ============================================================================

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        SubtitleWriter.cc
        SubtitleWriter.h
        UVCDevices.cc
        UVCDevices.h
        VideoManager.cc
        VideoManager.h
        VideoRecordingService.cc
        VideoRecordingService.h
        VideoStreamController.cc
        VideoStreamController.h
        VideoStreamResolver.cc
        VideoStreamResolver.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(VideoReceiver)
