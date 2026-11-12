target_sources(
    QGCUnitTests
    PRIVATE
        AutoSuspendGuardTest.cc
        AutoSuspendGuardTest.h
)

add_qgc_test(AutoSuspendGuardTest LABELS Unit Utilities)
