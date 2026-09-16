cmake_minimum_required(VERSION 3.25)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(QGCGPSNativeContractTests LANGUAGES CXX)
    enable_testing()
    add_subdirectory(../../../../../src/GPS/Driver/Protocols/Contracts NativeContracts)
endif()

add_executable(QGCGPSNativeContractsTest GPSNativeContractsTest.cc)
set_target_properties(
    QGCGPSNativeContractsTest
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
)
target_link_libraries(QGCGPSNativeContractsTest PRIVATE QGC::GPSNativeContracts)
add_test(NAME GPSNativeContractsTest COMMAND QGCGPSNativeContractsTest)
set_tests_properties(GPSNativeContractsTest PROPERTIES LABELS "Unit;GPS;Contracts" TIMEOUT 10)

set_target_properties(QGCGPSNativeContracts PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON)
add_custom_target(
    QGCGPSNativeContractTestHeaderChecks ALL
    DEPENDS QGCGPSNativeContracts_verify_interface_header_sets
    COMMENT "Verify Qt-free native public headers in isolation"
)
