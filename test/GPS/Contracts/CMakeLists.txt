cmake_minimum_required(VERSION 3.25)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(QGCGPSContractTests LANGUAGES CXX)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Core Test)
    add_subdirectory(../../../src/GPS/Contracts Contracts)
endif()

if(COMMAND add_qgc_test AND TARGET QGCGPSDriver)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSDriverConfigTest.cc GPSDriverConfigTest.h)
    add_qgc_test(GPSDriverConfigTest LABELS Unit)
endif()

qt_add_executable(QGCGPSBaseStationConfigTest GPSBaseStationConfigTest.cc)
set_target_properties(QGCGPSBaseStationConfigTest PROPERTIES AUTOMOC ON)
target_link_libraries(QGCGPSBaseStationConfigTest PRIVATE QGC::GPSContracts Qt6::Test)
add_test(NAME GPSBaseStationConfigTest COMMAND QGCGPSBaseStationConfigTest)
set_tests_properties(GPSBaseStationConfigTest PROPERTIES LABELS "Unit;GPS;Contracts" TIMEOUT 60 ENVIRONMENT
                                                         "QT_FATAL_WARNINGS=1"
)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set_target_properties(QGCGPSContracts PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON)
    add_custom_target(
        QGCGPSContractsHeaderChecks ALL
        DEPENDS QGCGPSContracts_verify_interface_header_sets
        COMMENT "Verify receiver contract public headers in isolation"
    )
    add_subdirectory(../Driver/Protocols/Contracts NativeContractsTests)
endif()
