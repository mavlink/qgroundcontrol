# ============================================================================
# Video Receiver Subsystem
# Video streaming backends (GStreamer and Qt Multimedia)
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        Core/VideoReceiver.cc
        Core/VideoReceiver.h
        Frame/VideoFrameDelivery.cc
        Frame/VideoFrameDelivery.h
        Frame/VideoStreamStats.cc
        Frame/VideoStreamStats.h
)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Multimedia)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/Frame
)

# ----------------------------------------------------------------------------
# Video Backend Implementations
# ----------------------------------------------------------------------------
add_subdirectory(GStreamer)
add_subdirectory(QtMultimedia)
add_subdirectory(Recording)
