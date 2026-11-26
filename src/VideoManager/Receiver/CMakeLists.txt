# ============================================================================
# Video Receiver API
# The backend-neutral VideoReceiver and VideoBackend, the reconnect policy, the stream statistics and screenshots;
# GStreamer/ implements them
# ============================================================================

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        KeyframeIntervalMeter.cc
        KeyframeIntervalMeter.h
        RecordingPlan.cc
        RecordingPlan.h
        StreamSupervisor.cc
        StreamSupervisor.h
        VideoReceiver.cc
        VideoReceiver.h
        VideoBackend.cc
        VideoBackend.h
        VideoBackendInit.cc
        VideoBackendInit.h
        VideoDecoderOptions.h
        VideoScreenshot.cc
        VideoScreenshot.h
        VideoStreamStats.h
        VideoStreamStatsMeter.cc
        VideoStreamStatsMeter.h
        VideoStreamStatsSampler.cc
        VideoStreamStatsSampler.h
        VideoStreamUri.cc
        VideoStreamUri.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
