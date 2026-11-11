target_sources(
    QGCUnitTests
    PRIVATE
        EdgeTriggeredCounterTest.cc
        EdgeTriggeredCounterTest.h
)

add_qgc_test(EdgeTriggeredCounterTest LABELS Unit Utilities)
