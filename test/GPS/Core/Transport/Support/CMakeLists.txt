# ============================================================================
# GPS Transport Test Support
# Developer-only raw stream recording, used by the transport tests and the hardware runner, plus the
# header-only link peers those tests share (a pseudo-terminal and a UDP port probe).
# ============================================================================

qt_add_library(QGCGPSTestTransports STATIC GPSRecordingTransport.cc GPSRecordingTransport.h PseudoTerminal.h
               UnusedUdpPort.h
)
target_compile_features(QGCGPSTestTransports PUBLIC cxx_std_20)
# Consumers include these as "Transport/Support/<header>", rooted at test/GPS/Core.
target_include_directories(QGCGPSTestTransports PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../..")
target_link_libraries(
    QGCGPSTestTransports
    PUBLIC QGCGPSCore Qt6::Core
    PRIVATE QGCLoggingCategory
)
