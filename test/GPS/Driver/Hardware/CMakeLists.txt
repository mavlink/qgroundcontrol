cmake_minimum_required(VERSION 3.25)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(QGCGPSHardwareTests LANGUAGES CXX)
    enable_testing()
    get_filename_component(QGC_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
    option(QGC_NO_SERIAL_LINK "Build without serial receiver support" OFF)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Core Network)
    if(NOT QGC_NO_SERIAL_LINK)
        find_package(Qt6 6.8 REQUIRED COMPONENTS SerialPort)
    endif()
    add_subdirectory("${QGC_SOURCE_ROOT}/src/Utilities/Logging/Category" LoggingCategory)
    add_subdirectory("${QGC_SOURCE_ROOT}/src/GPS/Receiver" Receiver)
    add_subdirectory("${QGC_SOURCE_ROOT}/src/GPS/Transport" Transport)
    add_subdirectory("${QGC_SOURCE_ROOT}/src/GPS/Driver" Driver)
endif()

if(NOT TARGET QGCGPSDriver OR NOT TARGET QGCGPSReceiverTransports)
    message(FATAL_ERROR "GPS hardware runner requires QGCGPSDriver and QGCGPSReceiverTransports")
endif()

add_executable(QGCGPSHardwareRunner GPSHardwareRunner.cc GPSEvidenceTransport.cc GPSEvidenceTransport.h
                                    "${CMAKE_CURRENT_SOURCE_DIR}/../ScriptedUBXReceiver.cc"
)
target_compile_features(QGCGPSHardwareRunner PRIVATE cxx_std_20)
target_include_directories(QGCGPSHardwareRunner PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(QGCGPSHardwareRunner PRIVATE QGCGPSDriver QGCGPSReceiverTransports QGC::Clock)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME GPSHardwareRunner.Scripted COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/check_runner.py"
                                                 "$<TARGET_FILE:QGCGPSHardwareRunner>" "$<BOOL:${QGC_NO_SERIAL_LINK}>"
)
set_tests_properties(GPSHardwareRunner.Scripted PROPERTIES LABELS "Unit;GPS" TIMEOUT 120)
