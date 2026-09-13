cmake_minimum_required(VERSION 3.25)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(QGCUtilityLibraryTests LANGUAGES CXX)
    enable_testing()
    set(_qgc_utilities_root "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/Utilities")
    option(QGC_UTILITY_WIRE_ONLY "Build only the Qt-free wire/checksum consumer" OFF)
    add_subdirectory("${_qgc_utilities_root}/Parsing/Wire" Wire)
    if(QGC_UTILITY_WIRE_ONLY)
        include(HeaderChecks.cmake)
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

include(HeaderChecks.cmake)
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
