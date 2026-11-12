# ============================================================================
# GPS Transport Test Support
# The reader of receiver recordings, the replay of a recorded receiver session and the replay of one as a dialogue
# that checks what the driver sends, used by the tests and the hardware runner, the checks of the pipe transport,
# which the application's suite and the standalone program (../Checks) run, plus the header-only link peers the
# transport tests share (a pseudo-terminal and a UDP port probe).
# ============================================================================

qt_add_library(
    QGCGPSTestTransports STATIC
    GPSRecordingDialogue.cc
    GPSRecordingDialogue.h
    GPSRecordingReader.cc
    GPSRecordingReader.h
    PipeTransportChecks.cc
    PipeTransportChecks.h
    PseudoTerminal.h
    ReplayGPSTransport.cc
    ReplayGPSTransport.h
    UnusedUdpPort.h
)
# The checks name their helpers as the other sources of the library may.
set_property(SOURCE PipeTransportChecks.cc PROPERTY SKIP_UNITY_BUILD_INCLUSION ON)
target_compile_features(QGCGPSTestTransports PUBLIC cxx_std_20)
# Consumers include these as "Transport/Support/<header>", rooted at test/GPS/Core.
target_include_directories(QGCGPSTestTransports PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../..")
# The dialogue replay runs GPSDriver, whose header is private to QGCGPSCore.
get_target_property(_qgc_gps_core_dir QGCGPSCore SOURCE_DIR)
target_include_directories(QGCGPSTestTransports PRIVATE "${_qgc_gps_core_dir}/Protocols")
target_link_libraries(
    QGCGPSTestTransports
    PUBLIC
        QGCGPSCore
        Qt6::Core
    PRIVATE QGCLoggingCategory
)
