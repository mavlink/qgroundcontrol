# ============================================================================
# Video Recordings Module
# Playback of the video recordings, with their telemetry subtitles
# ============================================================================

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        SubtitleTrack.cc
        SubtitleTrack.h
        VideoRecordingList.cc
        VideoRecordingList.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# QML Module for Video Recordings UI
# ----------------------------------------------------------------------------
qt_add_library(VideoRecordingsModule STATIC)

qt_add_qml_module(
    VideoRecordingsModule
    URI QGroundControl.AnalyzeView.VideoRecordings
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        VideoRecordingsPage.qml
    NO_PLUGIN
)

target_link_libraries(VideoRecordingsModule PRIVATE AnalyzeViewModule)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE VideoRecordingsModule)
