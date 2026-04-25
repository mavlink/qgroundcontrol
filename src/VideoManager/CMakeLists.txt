# ============================================================================
# Video Manager Module
# Handles video streaming and subtitle generation
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        Backend/VideoSourceCatalog.cc
        Backend/VideoSourceCatalog.h
        Backend/VideoSourceAvailability.cc
        Backend/VideoSourceAvailability.h
        Backend/VideoSourceController.cc
        Backend/VideoSourceController.h
        Backend/VideoSourceResolver.cc
        Backend/VideoSourceResolver.h
        Bridges/CameraControlBridge.cc
        Bridges/CameraControlBridge.h
        Bridges/VideoCameraBinder.cc
        Bridges/VideoCameraBinder.h
        Bridges/VideoSettingsBridge.cc
        Bridges/VideoSettingsBridge.h
        Recording/RecordingCoordinator.cc
        Recording/RecordingCoordinator.h
        Recording/SubtitleWriter.cc
        Recording/SubtitleWriter.h
        Recording/VideoMediaServices.cc
        Recording/VideoMediaServices.h
        Recording/VideoFileNaming.cc
        Recording/VideoFileNaming.h
        Recording/VideoRecordingPolicy.cc
        Recording/VideoRecordingPolicy.h
        Recording/VideoStorageCleaner.cc
        Recording/VideoStorageCleaner.h
        Stream/VideoIngestController.cc
        Stream/VideoIngestController.h
        Stream/VideoIngestSessionController.cc
        Stream/VideoIngestSessionController.h
        Stream/VideoBackendBootstrap.cc
        Stream/VideoBackendBootstrap.h
        Stream/VideoPlaybackRuntime.cc
        Stream/VideoPlaybackRuntime.h
        Stream/VideoSinkBinder.cc
        Stream/VideoSinkBinder.h
        Stream/VideoDiagnostics.cc
        Stream/VideoDiagnostics.h
        Stream/VideoManager.cc
        Stream/VideoManager.h
        Stream/VideoReceiverSession.cc
        Stream/VideoReceiverSession.h
        Recording/VideoRecordingService.cc
        Recording/VideoRecordingService.h
        Stream/VideoRestartPolicy.cc
        Stream/VideoRestartPolicy.h
        Stream/VideoSinkRouter.cc
        Stream/VideoSinkRouter.h
        Stream/VideoStreamAggregateMonitor.cc
        Stream/VideoStreamAggregateMonitor.h
        Stream/VideoStream.cc
        Stream/VideoStream.h
        Stream/VideoStreamSession.cc
        Stream/VideoStreamSession.h
        Stream/VideoStreamLifecycleController.cc
        Stream/VideoStreamLifecycleController.h
        Stream/VideoStreamLifecyclePolicy.cc
        Stream/VideoStreamLifecyclePolicy.h
        Stream/VideoStreamOrchestrator.cc
        Stream/VideoStreamOrchestrator.h
        Stream/VideoStreamStateMachine.cc
        Stream/VideoStreamStateMachine.h
        Stream/VideoUvcController.cc
        Stream/VideoUvcController.h
        Stream/VideoPlaybackInput.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Backend
    ${CMAKE_CURRENT_SOURCE_DIR}/Bridges
    ${CMAKE_CURRENT_SOURCE_DIR}/Recording
    ${CMAKE_CURRENT_SOURCE_DIR}/Stream
    ${CMAKE_CURRENT_SOURCE_DIR}/Utilities
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoReceiver/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoReceiver/Frame
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoReceiver/Recording
)

# ----------------------------------------------------------------------------
# Video QML Module
# ----------------------------------------------------------------------------
qt_add_library(VideoManagerModule STATIC)

# VideoStreamModel is a C++ type exposed to QML via QML_ELEMENT.
# It must be compiled into the QML module target for type registration.
target_sources(VideoManagerModule
    PRIVATE
        Stream/VideoStreamFsmState.h
        Stream/VideoStreamModel.cc
        Stream/VideoStreamModel.h
)
target_include_directories(VideoManagerModule PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Backend
    ${CMAKE_CURRENT_SOURCE_DIR}/Bridges
    ${CMAKE_CURRENT_SOURCE_DIR}/Recording
    ${CMAKE_CURRENT_SOURCE_DIR}/Stream
    ${CMAKE_CURRENT_SOURCE_DIR}/Utilities
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoReceiver
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoReceiver/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoReceiver/Frame
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoReceiver/Recording
)
target_link_libraries(VideoManagerModule
    PRIVATE
        Qt6::Core
        Qt6::Qml
        Qt6::Multimedia
)

qt_add_qml_module(VideoManagerModule
    URI QGroundControl.VideoManager
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        CameraZoomHandler.qml
        NoVideoPlaceholder.qml
        QGCVideoOutput.qml
        ThermalVideoView.qml
        VideoCompositor.qml
        VideoGridOverlay.qml
        VideoSizeCalculator.qml
        VideoStatsOverlay.qml
    SOURCES
        Stream/VideoStreamFsmState.h
        Stream/VideoStreamModel.h
    NO_PLUGIN
)

# ----------------------------------------------------------------------------
# Video Receiver Subsystem
# ----------------------------------------------------------------------------
add_subdirectory(VideoReceiver)
