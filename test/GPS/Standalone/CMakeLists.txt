cmake_minimum_required(VERSION 3.25)

include("${CMAKE_CURRENT_LIST_DIR}/../../LibraryBoundaryChecks.cmake")

if(COMMAND add_qgc_test AND NOT CMAKE_CROSSCOMPILING)
    if(NOT TARGET Python3::Interpreter)
        message(FATAL_ERROR "The GPS minimal-build matrix requires Python3::Interpreter")
    endif()
    foreach(component DriverReports ReceiverConfig Px4Adapter)
        add_test(
            NAME "CMake.GPSMinimal.${component}"
            COMMAND
                "$<TARGET_FILE:Python3::Interpreter>" "${CMAKE_CURRENT_SOURCE_DIR}/check_minimal_build.py"
                "--source-dir=${CMAKE_CURRENT_SOURCE_DIR}" "--build-root=${CMAKE_CURRENT_BINARY_DIR}/Minimal"
                "--component=${component}" "--generator=${CMAKE_GENERATOR}" "--platform=${CMAKE_GENERATOR_PLATFORM}"
                "--toolset=${CMAKE_GENERATOR_TOOLSET}" "--compiler=${CMAKE_CXX_COMPILER}" "--cmake=${CMAKE_COMMAND}"
                "--ctest=${CMAKE_CTEST_COMMAND}" "--config=$<CONFIG>"
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
        "Core;NMEA;Positioning;Transport;ReceiverTransports;RTCM;Corrections;NTRIP;Px4Adapter;ReceiverConfig"
        CACHE STRING "GPS library components to build and test"
    )
    option(QGC_NO_SERIAL_LINK "Build without serial receiver support" OFF)
    option(QGC_BUILD_ANDROID_SERIAL_TESTS "Build Linux-hosted Android serial compatibility tests" OFF)
    set(_gps_available_components
        Core
        NMEAProtocol
        NMEAUtils
        NMEA
        Positioning
        Transport
        ReceiverTransports
        RTCMFramer
        RTCM
        Corrections
        NTRIPHttp
        NTRIP
        DriverReports
        Px4Adapter
        ReceiverConfig
    )
    set(QGC_GPS_COMPONENTS_ENABLED ${QGC_GPS_COMPONENTS})
    foreach(component IN LISTS QGC_GPS_COMPONENTS_ENABLED)
        if(NOT component IN_LIST _gps_available_components)
            message(FATAL_ERROR "Unknown GPS component: ${component}")
        endif()
    endforeach()
    if("Px4Adapter" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        list(APPEND QGC_GPS_COMPONENTS_ENABLED DriverReports)
    endif()
    if("NMEA" IN_LIST QGC_GPS_COMPONENTS_ENABLED OR "Positioning" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        list(APPEND QGC_GPS_COMPONENTS_ENABLED Core)
    endif()
    if("NTRIP" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        list(APPEND QGC_GPS_COMPONENTS_ENABLED NTRIPHttp NMEAUtils RTCM)
    endif()
    if("NMEA" IN_LIST QGC_GPS_COMPONENTS_ENABLED OR "NMEAUtils" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
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
    if("RTCM" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        list(APPEND QGC_GPS_COMPONENTS_ENABLED RTCMFramer)
    endif()
    if("ReceiverTransports" IN_LIST QGC_GPS_COMPONENTS_ENABLED OR "NTRIPHttp" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        find_package(Qt6 6.8 REQUIRED COMPONENTS Network)
    endif()
    set(_qt_gps_components ${QGC_GPS_COMPONENTS_ENABLED})
    list(
        REMOVE_ITEM
        _qt_gps_components
        RTCMFramer
        NMEAProtocol
        DriverReports
        Px4Adapter
        ReceiverConfig
    )
    if(_qt_gps_components)
        find_package(Qt6 6.8 REQUIRED COMPONENTS Core)
        add_subdirectory("${_utilities_root}/Logging/Category" LoggingCategory)
    endif()
    if("DriverReports" IN_LIST QGC_GPS_COMPONENTS_ENABLED OR "ReceiverConfig" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        add_subdirectory("${_gps_root}/Receiver" Receiver EXCLUDE_FROM_ALL)
    endif()
    if("Px4Adapter" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        add_subdirectory("${_gps_root}/Driver/Px4" Px4Adapter)
    endif()
    if("Core" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        find_package(Qt6 6.8 REQUIRED COMPONENTS Positioning Test)
        add_subdirectory("${_utilities_root}/Timing" Timing)
        add_subdirectory("${_gps_root}/Core" Core)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/Timing/Support" TestTiming)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../Core" CoreTests)
    endif()
    if("NMEA" IN_LIST QGC_GPS_COMPONENTS_ENABLED OR "NTRIP" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        add_subdirectory("${_utilities_root}/IO" IO)
    endif()
    if("NMEAProtocol" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        add_subdirectory("${_gps_root}/NMEA" NMEA)
    endif()
    if("Positioning" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        add_subdirectory("${_gps_root}/PositionManager" Positioning)
    endif()
    if("Transport" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        add_subdirectory("${_gps_root}/Transport" Transport)
    endif()
    if("ReceiverTransports" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        find_package(Qt6 6.8 REQUIRED COMPONENTS Test)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../Transport" TransportTests)
    endif()
    if("RTCMFramer" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        add_subdirectory("${_gps_root}/RTCM" RTCM)
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

if(TARGET QGCGPSPx4Adapter)
    add_executable(QGCGPSPx4AdapterConsumer QGCGPSPx4AdapterConsumer.cc)
    set_target_properties(
        QGCGPSPx4AdapterConsumer
        PROPERTIES AUTOMOC OFF
                   AUTOUIC OFF
                   AUTORCC OFF
    )
    target_include_directories(QGCGPSPx4AdapterConsumer
                               PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/GPS/Driver/Px4"
                                       "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/GPS/Receiver"
    )
    target_link_libraries(QGCGPSPx4AdapterConsumer PRIVATE QGCGPSPx4Adapter QGCGPSDriverReports)
    add_test(NAME QGCGPSPx4AdapterConsumer COMMAND QGCGPSPx4AdapterConsumer)
    set_tests_properties(QGCGPSPx4AdapterConsumer PROPERTIES LABELS "Unit;GPS" TIMEOUT 10)
endif()

if(TARGET QGCClock)
    qgc_check_library_headers(QGCClock)
endif()
if(TARGET QGCGPSDriver)
    qgc_check_library_headers(QGCGPSDriver)
endif()

set(_receiver_consumer_targets)
foreach(component DriverReports ReceiverConfig)
    if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR OR component IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        list(APPEND _receiver_consumer_targets "QGCGPS${component}")
    endif()
endforeach()

foreach(
    target
    QGCGPSCore
    QGCGPSPositioning
    QGCGPSNMEA
    QGCGPSNMEAProtocol
    QGCGPSNMEAUtils
    QGCNetworkClient
    QGCGPSNTRIPHttp
    QGCGPSNTRIP
    QGCGPSTransport
    QGCGPSReceiverTransports
    QGCGPSRTCMFramer
    QGCGPSRTCM
    QGCGPSCorrections
    ${_receiver_consumer_targets}
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
    if("${target}" MATCHES "^QGCGPS(Core|Positioning|NMEA)$")
        target_link_libraries(${target}Consumer PRIVATE QGCTestTiming)
    endif()
endforeach()
