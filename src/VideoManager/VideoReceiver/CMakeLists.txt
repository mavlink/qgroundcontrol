# ============================================================================
# Video Receiver Subsystem
# Backend-neutral VideoReceiver interface and the GStreamer backend
# ============================================================================

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        VideoReceiver.cc
        VideoReceiver.h
        VideoBackend.cc
        VideoBackend.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(GStreamer)
