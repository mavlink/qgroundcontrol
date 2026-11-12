# ============================================================================
# GPS Receiver Qualification Runner
# Configures and observes a physical receiver for qualification evidence, replays receiver recordings, and serves a
# simulated receiver over TCP; its CTest drives it against a scripted receiver. Part of the QGC test build on Linux,
# where the application also runs the simulator's suite, and of test/GPS/Standalone without the application.
# ============================================================================

# The receiver the runner serves with --serve, as a library so that its suite runs it in process.
add_library(QGCGPSReceiverSimulator STATIC GPSReceiverSimulator.cc GPSReceiverSimulator.h)
target_compile_features(QGCGPSReceiverSimulator PUBLIC cxx_std_20)
target_link_libraries(
    QGCGPSReceiverSimulator
    PUBLIC
        QGCGPSCore
        QGCGPSProtocolTestSupport
        QGCGPSTestTransports
        Qt6::Network
)
if(TARGET QGCUnitTests)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSReceiverSimulator)
    qgc_add_gps_suites(Unit GPSReceiverSimulatorTest)
endif()

add_executable(QGCGPSHardwareRunner GPSHardwareRunner.cc GPSEvidenceTransport.cc GPSEvidenceTransport.h)
target_compile_features(QGCGPSHardwareRunner PRIVATE cxx_std_20)
target_link_libraries(
    QGCGPSHardwareRunner
    PRIVATE
        QGCGPSCore
        QGCGPSProtocolTestSupport
        QGCGPSReceiverSimulator
        QGCGPSTestTransports
        QGCLoggingCategory
)

find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_test(
        NAME GPSHardwareRunner.Scripted COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/check_runner.py"
                                                "$<TARGET_FILE:QGCGPSHardwareRunner>" "$<BOOL:${QGC_NO_SERIAL_LINK}>"
    )
    set_tests_properties(
        GPSHardwareRunner.Scripted
        PROPERTIES
            LABELS "Unit;GPS"
            TIMEOUT 120
    )
endif()
