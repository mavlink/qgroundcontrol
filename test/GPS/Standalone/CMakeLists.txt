cmake_minimum_required(VERSION 3.25)

include("${CMAKE_CURRENT_LIST_DIR}/../../LibraryBoundaryChecks.cmake")

if(COMMAND add_qgc_test AND NOT CMAKE_CROSSCOMPILING)
    if(NOT TARGET Python3::Interpreter)
        message(FATAL_ERROR "The GPS minimal-build matrix requires Python3::Interpreter")
    endif()
    foreach(
        component
        Receiver
        Transport
        NMEAProtocol
        RTCM
        MavlinkPacket
        Native
        Driver
    )
        add_test(
            NAME "CMake.GPSMinimal.${component}"
            COMMAND
                "$<TARGET_FILE:Python3::Interpreter>" "${CMAKE_CURRENT_SOURCE_DIR}/check_minimal_build.py"
                "--source-dir=${CMAKE_CURRENT_SOURCE_DIR}" "--build-root=${CMAKE_CURRENT_BINARY_DIR}/Minimal"
                "--component=${component}" "--generator=${CMAKE_GENERATOR}" "--platform=${CMAKE_GENERATOR_PLATFORM}"
                "--toolset=${CMAKE_GENERATOR_TOOLSET}" "--compiler=${CMAKE_CXX_COMPILER}" "--cmake=${CMAKE_COMMAND}"
                "--ctest=${CMAKE_CTEST_COMMAND}" "--config=$<CONFIG>" "--qt-dir=${Qt6_DIR}"
                "--cpm-source-cache=${CPM_SOURCE_CACHE}"
        )
        set_tests_properties("CMake.GPSMinimal.${component}" PROPERTIES LABELS "CMake;Unit;GPS" TIMEOUT 180)
    endforeach()
endif()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(QGCGPSLibraryTests LANGUAGES CXX)
    enable_testing()
    set(_gps_root "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/GPS")
    set(_utilities_root "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/Utilities")
    set(QGC_GPS_COMPONENTS
        "Core;NMEAPositioning;Positioning;Transport;ReceiverTransports;RTCM;MavlinkPacket"
        "Corrections;NTRIP;Receiver;Driver"
        CACHE STRING "GPS library components to build and test"
    )
    option(QGC_NO_SERIAL_LINK "Build without serial receiver support" OFF)
    option(QGC_BUILD_ANDROID_SERIAL_TESTS "Build Linux-hosted Android serial compatibility tests" OFF)
    set(_gps_available_components
        Core
        NMEAProtocol
        NMEAPositioning
        Positioning
        Transport
        ReceiverTransports
        RTCM
        MavlinkPacket
        Corrections
        NTRIPHttp
        NTRIP
        Receiver
        Native
        Driver
    )
    set(QGC_GPS_COMPONENTS_ENABLED ${QGC_GPS_COMPONENTS})
    foreach(component IN LISTS QGC_GPS_COMPONENTS_ENABLED)
        if(NOT component IN_LIST _gps_available_components)
            message(FATAL_ERROR "Unknown GPS component: ${component}")
        endif()
    endforeach()
    if("Driver" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        list(APPEND QGC_GPS_COMPONENTS_ENABLED Native Transport)
    endif()
    if("Native" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        list(APPEND QGC_GPS_COMPONENTS_ENABLED Receiver Transport NMEAProtocol RTCM)
    endif()
    if("NMEAPositioning" IN_LIST QGC_GPS_COMPONENTS_ENABLED OR "Positioning" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        list(APPEND QGC_GPS_COMPONENTS_ENABLED Core)
    endif()
    if("NTRIP" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        list(APPEND QGC_GPS_COMPONENTS_ENABLED NTRIPHttp NMEAProtocol RTCM)
        find_package(Qt6 6.8 REQUIRED COMPONENTS Positioning)
    endif()
    if("NMEAPositioning" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        list(APPEND QGC_GPS_COMPONENTS_ENABLED NMEAProtocol)
        find_package(Qt6 6.8 REQUIRED COMPONENTS Positioning)
    endif()
    if("ReceiverTransports" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        list(APPEND QGC_GPS_COMPONENTS_ENABLED Transport)
        if(NOT QGC_NO_SERIAL_LINK)
            find_package(Qt6 6.8 REQUIRED COMPONENTS SerialPort)
        endif()
    endif()
    if("Corrections" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        list(APPEND QGC_GPS_COMPONENTS_ENABLED RTCM)
    endif()
    if("ReceiverTransports" IN_LIST QGC_GPS_COMPONENTS_ENABLED OR "NTRIPHttp" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        find_package(Qt6 6.8 REQUIRED COMPONENTS Network)
    endif()
    find_package(Qt6 6.8 REQUIRED COMPONENTS Core)
    add_subdirectory("${_utilities_root}/Logging/Category" LoggingCategory)
    if("Receiver" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        add_subdirectory("${_gps_root}/Receiver" Receiver)
    endif()
    if("Core" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        find_package(Qt6 6.8 REQUIRED COMPONENTS Positioning Test)
        add_subdirectory("${_utilities_root}/Timing" Timing)
        add_subdirectory("${_gps_root}/Core" Core)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/Timing/Support" TestTiming)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../Core" CoreTests)
    endif()
    if("NMEAPositioning" IN_LIST QGC_GPS_COMPONENTS_ENABLED OR "NTRIP" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        add_subdirectory("${_utilities_root}/IO" IO)
    endif()
    if("NMEAProtocol" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        add_subdirectory("${_gps_root}/Driver/Protocols/NMEA" NMEAProtocol)
    endif()
    if("Positioning" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        add_subdirectory("${_gps_root}/Positioning" Positioning)
    elseif("NMEAPositioning" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        add_subdirectory("${_gps_root}/Positioning/NMEA" NMEAPositioning)
    endif()
    if("Transport" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        add_subdirectory("${_gps_root}/Transport" Transport)
    endif()
    if("ReceiverTransports" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        find_package(Qt6 6.8 REQUIRED COMPONENTS Test)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../Transport" TransportTests)
    endif()
    if("RTCM" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        add_subdirectory("${_gps_root}/Driver/Protocols/RTCM" RTCM)
    endif()
    if("MavlinkPacket" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        add_subdirectory("${_gps_root}/Corrections/MAVLink" MavlinkPacket)
        find_package(Qt6 6.8 REQUIRED COMPONENTS Test)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../Corrections/MAVLink" MavlinkPacketTests)
    endif()
    if("Native" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        add_subdirectory("${_gps_root}/Driver/Protocols" Native)
    endif()
    if("Driver" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        add_subdirectory("${_gps_root}/Driver" Driver)
    endif()
    if("RTCM" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        find_package(Qt6 6.8 REQUIRED COMPONENTS Test)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../RTCM" RTCMTests)
    endif()
    if("Corrections" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        add_subdirectory("${_gps_root}/Corrections" Corrections)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../Corrections" CorrectionsTests)
    endif()
    if("NTRIP" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        add_subdirectory("${_utilities_root}/Timing/Clock" Clock)
        add_subdirectory("${_utilities_root}/Network/Client" NetworkClient)
    endif()
    if("NTRIPHttp" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        add_subdirectory("${_gps_root}/NTRIP" NTRIP)
    endif()
    if("NTRIP" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../NTRIP/Standalone" NTRIPTests)
    endif()
endif()

if(TARGET QGCClock)
    qgc_check_library_headers(QGCClock)
endif()
if(TARGET QGCGPSNative)
    get_target_property(_native_targets QGCGPSNative QGC_GPS_NATIVE_TARGETS)
    foreach(target IN LISTS _native_targets)
        qgc_check_library_headers(${target})
    endforeach()
    add_executable(QGCGPSNativeConsumer QGCGPSNativeConsumer.cc)
    set_target_properties(
        QGCGPSNativeConsumer
        PROPERTIES AUTOMOC OFF
                   AUTOUIC OFF
                   AUTORCC OFF
    )
    target_link_libraries(QGCGPSNativeConsumer PRIVATE QGCGPSNative)
    add_test(NAME QGCGPSNativeConsumer COMMAND QGCGPSNativeConsumer)
    set_tests_properties(QGCGPSNativeConsumer PROPERTIES LABELS "Unit;GPS" TIMEOUT 10)
endif()

foreach(
    target
    QGCGPSCore
    QGCGPSPositioning
    QGCGPSNMEAPositioning
    QGCGPSNMEAProtocol
    QGCNetworkClient
    QGCGPSNTRIPHttp
    QGCGPSNTRIP
    QGCGPSTransport
    QGCGPSReceiverTransports
    QGCGPSRTCM
    QGCGPSMavlinkPacket
    QGCGPSCorrections
    QGCGPSDriver
    QGCGPSReceiver
)
    if(NOT TARGET ${target})
        continue()
    endif()
    qgc_add_library_consumer(
        ${target}
        SOURCE
        "${CMAKE_CURRENT_SOURCE_DIR}/${target}Consumer.cc"
        LABELS
        Unit
        GPS
        TIMEOUT
        10
    )
    if("${target}" MATCHES "^QGCGPS(Core|Positioning|NMEAPositioning)$")
        target_link_libraries(${target}Consumer PRIVATE QGCTestTiming)
    endif()
    if("${target}" STREQUAL "QGCGPSDriver")
        target_link_libraries(${target}Consumer PRIVATE QGCGPSTransport)
    endif()
endforeach()
