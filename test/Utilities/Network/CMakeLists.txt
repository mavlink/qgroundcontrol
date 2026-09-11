# ============================================================================
# Network Utilities Unit Tests
# Tests for networking helper functions
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME} PRIVATE QGCNetworkHelperTest.cc QGCNetworkHelperTest.h UdpIODeviceTest.cc
                                             UdpIODeviceTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(QGCNetworkHelperTest LABELS Unit Utilities Network)
add_qgc_test(UdpIODeviceTest LABELS Unit Utilities)

add_qgc_test(UdpForwarderTest LABELS Unit)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE UdpForwarderTest.cc UdpForwarderTest.h)
