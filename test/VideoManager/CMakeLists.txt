# ============================================================================
# VideoManager Tests
# ============================================================================

add_subdirectory(GStreamer)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        FakeVideoReceiver.cc
        FakeVideoReceiver.h
        RecordingSessionTest.cc
        RecordingSessionTest.h
        VideoManagerInitTest.cc
        VideoManagerInitTest.h
        VideoManagerIntegrationTest.cc
        VideoManagerIntegrationTest.h
        VideoStreamStateMachineTest.cc
        VideoStreamStateMachineTest.h
        VideoStreamTest.cc
        VideoStreamTest.h
)
