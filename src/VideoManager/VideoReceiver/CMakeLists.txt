# ============================================================================
# Video Receiver Subsystem
# Video streaming backends (GStreamer and Qt Multimedia)
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME} PRIVATE VideoReceiver.h)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# Video Backend Implementations
# ----------------------------------------------------------------------------
add_subdirectory(GStreamer)
add_subdirectory(QtMultimedia)
