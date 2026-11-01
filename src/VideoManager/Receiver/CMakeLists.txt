# ============================================================================
# Video Receiver API
# The backend-neutral VideoReceiver and VideoBackend, the stream statistics and screenshots; GStreamer/ implements them
# ============================================================================

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        SharedVideoFrame.cc
        SharedVideoFrame.h
        VideoReceiver.cc
        VideoReceiver.h
        VideoBackend.cc
        VideoBackend.h
        VideoDecoderOptions.h
        VideoScreenshot.cc
        VideoScreenshot.h
        VideoStreamStats.h
        VideoStreamStatsMeter.cc
        VideoStreamStatsMeter.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
