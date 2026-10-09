# ============================================================================
# GPS Receiver Qualification Runner
# Configures and observes a physical receiver for qualification evidence; its CTest drives it against a scripted
# receiver. Part of the QGC test build on Linux, and of test/GPS/Standalone without the application.
# ============================================================================

add_executable(QGCGPSHardwareRunner GPSHardwareRunner.cc GPSEvidenceTransport.cc GPSEvidenceTransport.h
                                    ReplayGPSTransport.cc ReplayGPSTransport.h
)
target_compile_features(QGCGPSHardwareRunner PRIVATE cxx_std_20)
target_link_libraries(QGCGPSHardwareRunner PRIVATE QGCGPSCore QGCGPSProtocolTestSupport QGCGPSTestTransports
                                                   QGCLoggingCategory
)

find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_test(NAME GPSHardwareRunner.Scripted
             COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/check_runner.py"
                     "$<TARGET_FILE:QGCGPSHardwareRunner>" "$<BOOL:${QGC_NO_SERIAL_LINK}>"
    )
    set_tests_properties(GPSHardwareRunner.Scripted PROPERTIES LABELS "Unit;GPS" TIMEOUT 120)
endif()
