# ============================================================================
# GPS Pipe Transport Checks
# The checks of the pipe transport (../Support/PipeTransportChecks) as a program without the unit test framework.
# Only test/GPS/Standalone builds it, to run the checks under its compiler and sanitizers: the application runs them
# in GPSPipeTransportTest.
# ============================================================================

add_executable(QGCGPSPipeTransportChecks GPSPipeTransportChecks.cc)
target_compile_features(QGCGPSPipeTransportChecks PRIVATE cxx_std_20)
target_link_libraries(QGCGPSPipeTransportChecks PRIVATE QGCGPSTestTransports)
set_target_properties(
    QGCGPSPipeTransportChecks
    PROPERTIES
        AUTOMOC OFF
        AUTOUIC OFF
        AUTORCC OFF
)
add_test(NAME GPSPipeTransportChecks COMMAND QGCGPSPipeTransportChecks)
set_tests_properties(
    GPSPipeTransportChecks
    PROPERTIES
        LABELS "Unit;GPS"
        TIMEOUT 120
)
