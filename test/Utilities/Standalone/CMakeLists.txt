cmake_minimum_required(VERSION 3.25)

# Verify exported headers and link a consumer using only its owning target.
function(qgc_check_utility_consumer target)
    set_target_properties(${target} PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON)
    add_custom_target(
        ${target}HeaderChecks ALL
        DEPENDS ${target}_verify_interface_header_sets
        COMMENT "Verify ${target} public headers in isolation"
    )

    add_executable(${target}Consumer "${CMAKE_CURRENT_SOURCE_DIR}/Consumers/${target}.cc")
    set_target_properties(
        ${target}Consumer
        PROPERTIES AUTOMOC OFF
                   AUTOUIC OFF
                   AUTORCC OFF
    )
    target_link_libraries(${target}Consumer PRIVATE ${target})
    add_test(NAME ${target}Consumer COMMAND ${target}Consumer)
    set_tests_properties(${target}Consumer PROPERTIES LABELS "Unit;Utilities" TIMEOUT 30)
endfunction()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(QGCUtilityLibraryTests LANGUAGES CXX)
    enable_testing()
    set(_qgc_utilities_root "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/Utilities")
    option(QGC_UTILITY_WIRE_ONLY "Build only the Qt-free wire/checksum consumer" OFF)
    add_subdirectory("${_qgc_utilities_root}/Parsing/Wire" Wire)
    if(QGC_UTILITY_WIRE_ONLY)
        qgc_check_utility_consumer(QGCWire)
        return()
    endif()
    find_package(Qt6 6.8 REQUIRED COMPONENTS Core Network Test)
    add_subdirectory("${_qgc_utilities_root}/Logging/Category" LoggingCategory)
    add_subdirectory("${_qgc_utilities_root}/IO" IO)
    add_subdirectory("${_qgc_utilities_root}/Timing" Timing)
    add_subdirectory("${_qgc_utilities_root}/Network/IO" NetworkIO)
    add_subdirectory("${_qgc_utilities_root}/Parsing/Json" JsonValidation)
endif()

add_executable(QGCUtilityLibraryTest UtilityLibraryTest.cc)
set_target_properties(QGCUtilityLibraryTest PROPERTIES AUTOMOC ON)
target_link_libraries(
    QGCUtilityLibraryTest
    PRIVATE QGCIO
            QGCTiming
            QGCNetworkIO
            QGCWire
            QGCJsonValidation
            QGCLoggingCategory
            Qt6::Test
)
add_test(NAME UtilityLibraryTest COMMAND QGCUtilityLibraryTest)
set_tests_properties(UtilityLibraryTest PROPERTIES LABELS "Unit;Utilities" TIMEOUT 30)

foreach(target QGCWire QGCIO QGCTiming QGCNetworkIO QGCLoggingCategory QGCJsonValidation)
    qgc_check_utility_consumer(${target})
endforeach()

find_package(Threads REQUIRED)
add_executable(TimestampedByteBufferTest TimestampedByteBufferTest.cc)
set_target_properties(TimestampedByteBufferTest PROPERTIES AUTOMOC ON)
target_compile_features(TimestampedByteBufferTest PRIVATE cxx_std_20)
target_link_libraries(TimestampedByteBufferTest PRIVATE QGCIO Qt6::Test Threads::Threads)
add_test(NAME TimestampedByteBufferTest COMMAND TimestampedByteBufferTest)
set_tests_properties(TimestampedByteBufferTest PROPERTIES LABELS "Unit;Utilities" TIMEOUT 30)
