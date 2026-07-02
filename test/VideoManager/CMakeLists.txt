# ============================================================================
# VideoManager Tests
# ============================================================================

add_subdirectory(GStreamer)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        VideoSettingsTest.cc
        VideoSettingsTest.h
        VideoManagerInitTest.cc
        VideoManagerInitTest.h
)

add_qgc_test(VideoSettingsTest LABELS Unit)
add_qgc_test(VideoManagerInitTest LABELS Unit)
