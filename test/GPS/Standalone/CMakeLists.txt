cmake_minimum_required(VERSION 3.25)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(QGCGPSLibraryTests LANGUAGES CXX)
    enable_testing()
    set(_gps_root "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/GPS")
    set(_utilities_root "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/Utilities")
    add_subdirectory("${_gps_root}/Driver/Protocols/Contracts" NativeContracts)
    option(QGC_GPS_NATIVE_ONLY "Check GPS protocol contracts without Qt" OFF)
    option(QGC_GPS_CORE_ONLY "Check GPS Core and Models without Qt receiver contracts" OFF)
    if(NOT QGC_GPS_NATIVE_ONLY)
        find_package(Qt6 6.8 REQUIRED COMPONENTS Core Positioning)
        add_subdirectory("${_utilities_root}/Logging/Category" LoggingCategory)
        add_subdirectory("${_utilities_root}/Timing" Timing)
        if(NOT QGC_GPS_CORE_ONLY)
            add_subdirectory("${_gps_root}/Contracts" Contracts)
        endif()
        add_subdirectory("${_gps_root}/Core" Core)
        add_subdirectory("${_gps_root}/Models" Models)
    endif()
endif()

set_target_properties(QGCClock PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON)
add_custom_target(
    QGCClockHeaderChecks ALL
    DEPENDS QGCClock_verify_interface_header_sets
    COMMENT "Verify Qt-free clock public headers in isolation"
)
set(_consumer_targets QGCGPSNativeContracts)
if(NOT QGC_GPS_NATIVE_ONLY)
    if(NOT QGC_GPS_CORE_ONLY)
        list(APPEND _consumer_targets QGCGPSContracts)
    endif()
    list(APPEND _consumer_targets QGCGPSCore QGCGPSModels)
endif()
foreach(target IN LISTS _consumer_targets)
    set_target_properties(${target} PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON)
    add_custom_target(
        ${target}HeaderChecks ALL
        DEPENDS ${target}_verify_interface_header_sets
        COMMENT "Verify ${target} public headers in isolation"
    )
    if("${target}" STREQUAL "QGCGPSContracts")
        continue() # GPSContractsLinkTest already exercises this library independently.
    endif()
    add_executable(${target}Consumer ${target}Consumer.cc)
    target_link_libraries(${target}Consumer PRIVATE ${target})
    add_test(NAME ${target}Consumer COMMAND ${target}Consumer)
    set_tests_properties(${target}Consumer PROPERTIES LABELS "Unit;GPS;Contracts" TIMEOUT 10)
endforeach()
