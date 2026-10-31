# ============================================================================
# Video Receiver API
# The backend-neutral VideoReceiver, the VideoBackend facade and the stream statistics; GStreamer/ implements them
# ============================================================================

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        VideoReceiver.cc
        VideoReceiver.h
        VideoBackend.cc
        VideoBackend.h
        VideoDecoderOptions.h
        VideoStreamStats.h
        VideoStreamStatsMeter.cc
        VideoStreamStatsMeter.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
