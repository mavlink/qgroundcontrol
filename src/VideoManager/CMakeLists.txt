# ============================================================================
# Video Manager Module
# Handles video streaming and subtitle generation
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        Backend/VideoBackendRegistry.cc
        Backend/VideoBackendRegistry.h
        Backend/VideoReceiverFactory.cc
        Backend/VideoReceiverFactory.h
        Backend/VideoSourceResolver.cc
        Backend/VideoSourceResolver.h
        Bridges/CameraControlBridge.cc
        Bridges/CameraControlBridge.h
        Bridges/VideoSettingsBridge.cc
        Bridges/VideoSettingsBridge.h
        Recording/RecordingCoordinator.cc
        Recording/RecordingCoordinator.h
        Recording/SubtitleWriter.cc
        Recording/SubtitleWriter.h
        Recording/VideoFileNaming.cc
        Recording/VideoFileNaming.h
        Recording/VideoRecordingPolicy.cc
        Recording/VideoRecordingPolicy.h
        Recording/VideoStorageCleaner.cc
        Recording/VideoStorageCleaner.h
        Stream/VideoDiagnostics.cc
        Stream/VideoDiagnostics.h
        Stream/VideoManager.cc
        Stream/VideoManager.h
        Stream/VideoReceiverSession.cc
        Stream/VideoReceiverSession.h
        Stream/VideoRecorderSessionController.cc
        Stream/VideoRecorderSessionController.h
        Stream/VideoRestartPolicy.cc
        Stream/VideoRestartPolicy.h
        Stream/VideoSinkRouter.cc
        Stream/VideoSinkRouter.h
        Stream/VideoStream.cc
        Stream/VideoStream.h
        Stream/VideoStreamOrchestrator.cc
        Stream/VideoStreamOrchestrator.h
        Stream/VideoStreamStateMachine.cc
        Stream/VideoStreamStateMachine.h
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
