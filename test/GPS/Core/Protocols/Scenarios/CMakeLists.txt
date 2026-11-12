# ============================================================================
# GPS Golden Transcripts
# The checks of the golden transcripts (../golden) as a program without the unit test framework. Only
# test/GPS/Standalone builds it, to run the checks under its compiler and sanitizers: the application runs them in
# GPSGoldenTranscriptTest. The scenarios and the checks are sources of QGCGPSProtocolTestSupport (../Support).
# ============================================================================

add_executable(QGCGPSGoldenTranscripts GPSGoldenTranscripts.cc)
target_compile_features(QGCGPSGoldenTranscripts PRIVATE cxx_std_20)
target_link_libraries(QGCGPSGoldenTranscripts PRIVATE QGCGPSProtocolTestSupport)
set_target_properties(
    QGCGPSGoldenTranscripts
    PROPERTIES
        AUTOMOC OFF
        AUTOUIC OFF
        AUTORCC OFF
)
add_test(NAME GPSGoldenTranscripts COMMAND QGCGPSGoldenTranscripts)
set_tests_properties(
    GPSGoldenTranscripts
    PROPERTIES
        LABELS "Unit;GPS"
        TIMEOUT 60
)
