# ============================================================================
# Video Recordings Unit Tests
# Tests for the recordings list and the telemetry subtitle reader
# ============================================================================

target_sources(
    QGCUnitTests
    PRIVATE
        SubtitleTrackTest.cc
        SubtitleTrackTest.h
        VideoRecordingListTest.cc
        VideoRecordingListTest.h
)

add_qgc_test(SubtitleTrackTest LABELS Unit AnalyzeView)
add_qgc_test(VideoRecordingListTest LABELS Unit AnalyzeView)
