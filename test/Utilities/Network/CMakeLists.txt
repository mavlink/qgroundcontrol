# ============================================================================
# Network Utilities Unit Tests
# Tests for networking helper functions
# ============================================================================

target_sources(
    QGCUnitTests
    PRIVATE
        QGCNetworkAvailabilityMonitorTest.cc
        QGCNetworkAvailabilityMonitorTest.h
        QGCNetworkHelperTest.cc
        QGCNetworkHelperTest.h
        QGCNetworkRedactionTest.cc
        QGCNetworkRedactionTest.h
        UdpForwarderTest.cc
        UdpForwarderTest.h
)

target_include_directories(QGCUnitTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(QGCNetworkAvailabilityMonitorTest LABELS Unit Utilities)
add_qgc_test(QGCNetworkHelperTest LABELS Unit Utilities Network)
add_qgc_test(QGCNetworkRedactionTest LABELS Unit Utilities)

add_qgc_test(UdpForwarderTest LABELS Unit Utilities)
