# ============================================================================
# Android GPS Compatibility Test
# SerialGPSTransport as Android builds it, over the Android serial port that test/Android compiles for the host.
# It joins that library, whose test-only names of QSerialPort it has to share.
# ============================================================================

target_sources(
    QGCAndroidSerialTestBackend
    PRIVATE
        AndroidGPSCompatibilityTest.cc
        AndroidGPSCompatibilityTest.h
        ${CMAKE_SOURCE_DIR}/src/GPS/Core/Transport/SerialGPSTransport.cc
)
target_compile_definitions(
    QGCAndroidSerialTestBackend
    PRIVATE
        SerialGPSTransport=QGCAndroidTestGPSTransport
        SerialGPSTransportLog=QGCAndroidTestGPSTransportLog
)
target_include_directories(QGCAndroidSerialTestBackend PRIVATE ${CMAKE_SOURCE_DIR}/src/GPS/Core/Transport)
target_link_libraries(QGCAndroidSerialTestBackend PRIVATE QGCGPSCore)
add_qgc_test(AndroidGPSCompatibilityTest LABELS Unit GPS)
