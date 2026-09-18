cmake_minimum_required(VERSION 3.25)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(QGCGPSLibraryTests LANGUAGES CXX)
    enable_testing()
    set(_gps_root "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/GPS")
    set(_utilities_root "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/Utilities")
    set(QGC_GPS_COMPONENTS
        "Core;NMEA;Positioning;Transport;ReceiverTransports;RTCM;Corrections"
        CACHE STRING "GPS library components to build and test"
    )
    option(QGC_NO_SERIAL_LINK "Build without serial receiver support" OFF)
    option(QGC_BUILD_ANDROID_SERIAL_TESTS "Build Linux-hosted Android serial compatibility tests" OFF)
    set(QGC_GPS_COMPONENTS_ENABLED ${QGC_GPS_COMPONENTS})
    foreach(component IN LISTS QGC_GPS_COMPONENTS_ENABLED)
        if(NOT component MATCHES
           "^(Core|NMEA(Protocol)?|Positioning|Transport|ReceiverTransports|RTCM(Framer)?|Corrections)$"
        )
            message(FATAL_ERROR "Unknown GPS component: ${component}")
        endif()
    endforeach()
    if("NMEA" IN_LIST QGC_GPS_COMPONENTS_ENABLED OR "Positioning" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        list(APPEND QGC_GPS_COMPONENTS_ENABLED Core)
    endif()
    if("NMEA" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        list(APPEND QGC_GPS_COMPONENTS_ENABLED NMEAProtocol)
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
    if("ReceiverTransports" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        find_package(Qt6 6.8 REQUIRED COMPONENTS Network)
    endif()
    set(_qt_gps_components ${QGC_GPS_COMPONENTS_ENABLED})
    list(REMOVE_ITEM _qt_gps_components RTCMFramer NMEAProtocol)
    if(_qt_gps_components)
        find_package(Qt6 6.8 REQUIRED COMPONENTS Core)
        add_subdirectory("${_utilities_root}/Logging/Category" LoggingCategory)
    endif()
    if("Core" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
        find_package(Qt6 6.8 REQUIRED COMPONENTS Positioning Test)
        add_subdirectory("${_utilities_root}/Timing" Timing)
        add_subdirectory("${_gps_root}/Core" Core)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/Timing/Support" TestTiming)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../Core" CoreTests)
    endif()
    if("NMEA" IN_LIST QGC_GPS_COMPONENTS_ENABLED)
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
endif()

if(TARGET QGCClock)
    set_target_properties(QGCClock PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON)
    add_custom_target(
        QGCClockHeaderChecks ALL
        DEPENDS QGCClock_verify_interface_header_sets
        COMMENT "Verify Qt-free clock public headers in isolation"
    )
endif()
foreach(
    target
    QGCGPSCore
    QGCGPSPositioning
    QGCGPSNMEA
    QGCGPSNMEAProtocol
    QGCGPSTransport
    QGCGPSReceiverTransports
    QGCGPSRTCMFramer
    QGCGPSRTCM
    QGCGPSCorrections
)
    if(NOT TARGET ${target})
        continue()
    endif()
    set_target_properties(${target} PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON)
    add_custom_target(
        ${target}HeaderChecks ALL
        DEPENDS ${target}_verify_interface_header_sets
        COMMENT "Verify ${target} public headers in isolation"
    )
    add_executable(${target}Consumer "${CMAKE_CURRENT_SOURCE_DIR}/${target}Consumer.cc")
    set_target_properties(
        ${target}Consumer
        PROPERTIES AUTOMOC OFF
                   AUTOUIC OFF
                   AUTORCC OFF
    )
    target_link_libraries(${target}Consumer PRIVATE ${target})
    add_test(NAME ${target}Consumer COMMAND ${target}Consumer)
    set_tests_properties(${target}Consumer PROPERTIES LABELS "Unit;GPS" TIMEOUT 10)
    if("${target}" MATCHES "^QGCGPS(Core|Positioning|NMEA)$")
        target_link_libraries(${target}Consumer PRIVATE QGCTestTiming)
    endif()
endforeach()
