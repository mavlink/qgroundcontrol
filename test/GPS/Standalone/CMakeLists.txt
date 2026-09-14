cmake_minimum_required(VERSION 3.25)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(QGCGPSLibraryTests LANGUAGES CXX)
    enable_testing()
    set(_gps_root "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/GPS")
    set(_utilities_root "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/Utilities")
    find_package(Qt6 6.8 REQUIRED COMPONENTS Core Positioning)
    add_subdirectory("${_utilities_root}/Logging/Category" LoggingCategory)
    add_subdirectory("${_utilities_root}/Timing" Timing)
    add_subdirectory("${_gps_root}/Core" Core)
    add_subdirectory("${_gps_root}/PositionManager" Positioning)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/Timing/Support" TestTiming)
endif()

set_target_properties(QGCClock PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON)
add_custom_target(
    QGCClockHeaderChecks ALL
    DEPENDS QGCClock_verify_interface_header_sets
    COMMENT "Verify Qt-free clock public headers in isolation"
)
foreach(target QGCGPSCore QGCGPSPositioning)
    set_target_properties(${target} PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON)
    add_custom_target(
        ${target}HeaderChecks ALL
        DEPENDS ${target}_verify_interface_header_sets
        COMMENT "Verify ${target} public headers in isolation"
    )
    add_executable(${target}Consumer ${target}Consumer.cc)
    target_link_libraries(${target}Consumer PRIVATE ${target} QGCTestTiming)
    add_test(NAME ${target}Consumer COMMAND ${target}Consumer)
    set_tests_properties(${target}Consumer PROPERTIES LABELS "Unit;GPS" TIMEOUT 10)
endforeach()
