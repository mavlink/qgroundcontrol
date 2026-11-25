target_sources(
    QGCUnitTests
    PRIVATE
        PlatformTest.cc
        PlatformTest.h
)

# RunGuard and SignalHandler only available on desktop platforms
if(NOT ANDROID AND NOT IOS)
    add_qgc_test(PlatformTest LABELS Unit Utilities)
    add_qgc_tests(
        RunGuardTest
        SignalHandlerTest
        LABELS Unit Utilities
    )
endif()
