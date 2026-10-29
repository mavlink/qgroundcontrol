cmake_minimum_required(VERSION 3.28)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(QGCUomRegistrationTests LANGUAGES CXX)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Core Network Sql Concurrent Test)
    add_subdirectory(../../../../src/Vehicle/UOM/Registration RegistrationCore)
endif()

if(NOT DEFINED QGC_TEST_TIMEOUT_UNIT)
    set(QGC_TEST_TIMEOUT_UNIT 60)
endif()

foreach(suite IN ITEMS UomRegistrationCoreTest UomRegistrationWorkflowTest)
    add_executable(QGC${suite} ${suite}.cc ${suite}.h ../../../Portable/PortableTest.h)
    set_target_properties(
        QGC${suite}
        PROPERTIES
            AUTOMOC ON
    )
    target_compile_definitions(QGC${suite} PRIVATE QGC_PORTABLE_TEST)
    target_include_directories(
        QGC${suite}
        PRIVATE
            ../../../Portable
            ../../../UnitTestFramework
    )
    target_link_libraries(
        QGC${suite}
        PRIVATE
            QGCUomRegistrationCore
            Qt6::Test
    )
    add_test(NAME ${suite} COMMAND QGC${suite})
    set_tests_properties(
        ${suite}
        PROPERTIES
            LABELS "Unit;UOM"
            TIMEOUT ${QGC_TEST_TIMEOUT_UNIT}
            ENVIRONMENT "QT_FATAL_CRITICALS=1"
    )
endforeach()

if(TARGET QGCUnitTests)
    target_sources(
        QGCUnitTests
        PRIVATE
            VehicleUomRegistrationManagerTest.cc
            VehicleUomRegistrationManagerTest.h
    )
    add_qgc_test(VehicleUomRegistrationManagerTest LABELS Unit Vehicle UOM)
endif()
