# ============================================================================
# VideoRecorder — unified recording interface and implementations
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        FrameDeliveryRecorder.cc
        FrameDeliveryRecorder.h
        VideoRecorder.cc
        VideoRecorder.h
        RecordingOrphanScanner.cc
        RecordingOrphanScanner.h
        RecordingSession.cc
        RecordingSession.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
