# ============================================================================
# GPS Protocol Test Support
# The application-free receiver models, scripted links, clock and packet builders that the protocol suites, the
# hardware runner and the fuzzer share. Consumers include them as "Protocols/Support/<header>", rooted at
# test/GPS/Core; the helpers that need the unit test framework are sources of the application (../CMakeLists.txt).
# ============================================================================

add_library(
    QGCGPSProtocolTestSupport STATIC
    AshtechReceiverModel.h
    DetectionReceiver.h
    FemtoReceiverModel.h
    GPSEventSummary.h
    GPSModelViolations.h
    GPSProtocolTestData.h
    GPSTestClock.h
    ProtocolTestPackets.h
    QuectelReceiverModel.h
    ReceiverBench.h
    ReceiverEventQueue.h
    SBFReceiverModel.h
    ScriptedFaults.h
    ScriptedReceiver.cc
    ScriptedReceiver.h
    UBXReceiverModel.cc
    UBXReceiverModel.h
    UnicoreReceiverModel.h
)
target_compile_features(QGCGPSProtocolTestSupport PUBLIC cxx_std_20)
target_include_directories(QGCGPSProtocolTestSupport PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../..")
# The protocol headers are private to QGCGPSCore; the protocol suites, the hardware runner and the fuzzer include them
# through this library.
get_target_property(_qgc_gps_core_dir QGCGPSCore SOURCE_DIR)
target_include_directories(QGCGPSProtocolTestSupport PUBLIC "${_qgc_gps_core_dir}/Protocols")
# GPSProtocolTestData.h locates the golden, corpus, fixture and transcript data from test/GPS/Core/Protocols.
get_filename_component(_qgc_gps_protocol_test_dir "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
target_compile_definitions(QGCGPSProtocolTestSupport PUBLIC GPS_PROTOCOL_TEST_DIR="${_qgc_gps_protocol_test_dir}")
target_link_libraries(QGCGPSProtocolTestSupport PUBLIC QGCGPSCore)
