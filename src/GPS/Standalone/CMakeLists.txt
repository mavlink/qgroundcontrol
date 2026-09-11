cmake_minimum_required(VERSION 3.25)
project(QGCGPSLibraries LANGUAGES CXX)
include(CTest)
option(QGC_NO_SERIAL_LINK "Build without serial transports" OFF)
include(../Libraries.cmake)
if(BUILD_TESTING)
    include(HeaderChecks.cmake)
    foreach(boundary_case valid direct transitive alias header late-target)
        add_test(
            NAME GPSLibraryBoundary-${boundary_case}
            COMMAND
                "${CMAKE_COMMAND}" "-DFIXTURE=${CMAKE_CURRENT_SOURCE_DIR}/../../../test/GPS/Standalone/BoundaryFixture"
                "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/boundary/${boundary_case}"
                "-DQGC_GPS_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/.." "-DCASE=${boundary_case}" -P
                "${CMAKE_CURRENT_SOURCE_DIR}/../../../test/GPS/Standalone/CheckBoundary.cmake"
        )
        set_tests_properties(GPSLibraryBoundary-${boundary_case} PROPERTIES LABELS "Unit;GPS;Library;CMake" TIMEOUT 30)
    endforeach()
    if(TARGET Qt6::Core)
        find_package(Qt6 REQUIRED COMPONENTS Test)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../test/GPS/Standalone" LibraryTests)
    endif()
    if(TARGET QGCGPSContracts)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../test/GPS/Contracts" ContractsTest)
    endif()
    if(TARGET QGCGPSRecording)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../test/GPS/Recording" RecordingTest)
    endif()
    if(TARGET QGCGPSNMEAProtocol AND NOT TARGET QGCGPSNative)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../test/GPS/Driver/Protocols/NMEA" NMEAProtocolTest)
    endif()
    if(TARGET QGCGPSNative)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../test/GPS/Driver/Protocols" NativeTest)
    endif()
    if(TARGET QGCGPSReceiver
       AND TARGET QGCGPSRecordingController
       AND TARGET QGCGPSNMEA
       AND TARGET QGCGPSNTRIPSession
    )
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../test/GPS/Replay" ReplayTest)
    endif()
endif()
if(TARGET QGCGPSDriver)
    add_executable(GPSLibraryConsumer LibraryConsumer.cc)
    target_link_libraries(GPSLibraryConsumer PRIVATE QGC::GPSDriver)
    if(BUILD_TESTING)
        add_test(NAME GPSLibraryConsumer COMMAND GPSLibraryConsumer)
        set_tests_properties(GPSLibraryConsumer PROPERTIES LABELS "Unit;GPS;Library" TIMEOUT 10)
    endif()
endif()
