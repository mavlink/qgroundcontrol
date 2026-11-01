# ============================================================================
# Video Manager Module
# VideoManager, the QML facade, and its telemetry overlay; Streams: per-stream control and the settings resolver;
# Recording: file naming, storage and subtitles; UVC: local cameras; Receiver: the backend-neutral receiver API;
# GStreamer: the network backend; QtMultimedia: the QMediaPlayer receiver, without GStreamer or by choice; Qml: the
# QGroundControl.VideoManager QML module.
# ============================================================================

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        VideoManager.cc
        VideoManager.h
        VideoTelemetryOverlay.cc
        VideoTelemetryOverlay.h
        Recording/QtVideoRecorder.cc
        Recording/QtVideoRecorder.h
        Recording/SubtitleWriter.cc
        Recording/SubtitleWriter.h
        Recording/TelemetryBarValues.cc
        Recording/TelemetryBarValues.h
        Recording/VideoRecordingService.cc
        Recording/VideoRecordingService.h
        Streams/VideoStreamController.cc
        Streams/VideoStreamController.h
        Streams/VideoStreamResolver.cc
        Streams/VideoStreamResolver.h
        UVC/UVCCameraController.cc
        UVC/UVCCameraController.h
        UVC/UVCDevices.cc
        UVC/UVCDevices.h
        UVC/UVCFrameRelay.cc
        UVC/UVCFrameRelay.h
        UVC/UVCStreamMonitor.cc
        UVC/UVCStreamMonitor.h
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
add_subdirectory(QtMultimedia)
add_subdirectory(GStreamer)
add_subdirectory(Qml)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE VideoManagerModule)
