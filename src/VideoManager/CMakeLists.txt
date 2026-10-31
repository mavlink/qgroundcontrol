# ============================================================================
# Video Manager Module
# VideoManager, the QML facade; Streams: per-stream control and the settings resolver; Recording: file naming, storage
# and subtitles; UVC: local cameras; Receiver: the backend-neutral receiver API; GStreamer: the network backend.
# ============================================================================

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        VideoManager.cc
        VideoManager.h
        Recording/SubtitleWriter.cc
        Recording/SubtitleWriter.h
        Recording/VideoRecordingService.cc
        Recording/VideoRecordingService.h
        Streams/VideoStreamController.cc
        Streams/VideoStreamController.h
        Streams/VideoStreamResolver.cc
        Streams/VideoStreamResolver.h
        UVC/UVCDevices.cc
        UVC/UVCDevices.h
)

target_include_directories(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Recording
        ${CMAKE_CURRENT_SOURCE_DIR}/Streams
        ${CMAKE_CURRENT_SOURCE_DIR}/UVC
)

add_subdirectory(Receiver)
add_subdirectory(GStreamer)
