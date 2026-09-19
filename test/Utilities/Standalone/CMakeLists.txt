cmake_minimum_required(VERSION 3.25)

include("${CMAKE_CURRENT_LIST_DIR}/../../LibraryBoundaryChecks.cmake")

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(QGCUtilityLibraryTests LANGUAGES CXX)
    enable_testing()
    set(_qgc_utilities_root "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/Utilities")
    option(QGC_UTILITY_CRC_ONLY "Build only the Qt-free CRC consumer" OFF)
    option(QGC_UTILITY_QT_FREE_ONLY "Build only Qt-free utility consumers" OFF)
endif()

add_executable(CRC32Consumer Consumers/CRC32.cc)
set_target_properties(
    CRC32Consumer
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
)
target_compile_features(CRC32Consumer PRIVATE cxx_std_20)
target_include_directories(CRC32Consumer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/Utilities/Math")
add_test(NAME CRC32Consumer COMMAND CRC32Consumer)
set_tests_properties(CRC32Consumer PROPERTIES LABELS "Unit;Utilities" TIMEOUT 30)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    if(QGC_UTILITY_CRC_ONLY)
        return()
    endif()
    add_subdirectory("${_qgc_utilities_root}/Parsing/Wire" Wire)
endif()

qgc_add_library_consumer(
    QGCWire
    SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/Consumers/QGCWire.cc"
    LABELS
    Unit
    Utilities
    TIMEOUT
    30
)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    if(QGC_UTILITY_QT_FREE_ONLY)
        return()
    endif()
    find_package(Qt6 6.8 REQUIRED COMPONENTS Core Network Test)
    add_subdirectory("${_qgc_utilities_root}/Logging/Category" LoggingCategory)
    add_subdirectory("${_qgc_utilities_root}/IO" IO)
    add_subdirectory("${_qgc_utilities_root}/Timing" Timing)
    add_subdirectory("${_qgc_utilities_root}/Network/IO" NetworkIO)
    add_subdirectory("${_qgc_utilities_root}/Parsing/Json" JsonValidation)
endif()

if(NOT TARGET QGCTestTiming)
    add_subdirectory(../Timing/Support TestTiming)
endif()

add_executable(QGCUtilityLibraryTest UtilityLibraryTest.cc)
set_target_properties(QGCUtilityLibraryTest PROPERTIES AUTOMOC ON)
target_link_libraries(QGCUtilityLibraryTest PRIVATE QGCIO QGCTestTiming QGCNetworkIO QGCJsonValidation
                                                    QGCLoggingCategory Qt6::Test
)
add_test(NAME UtilityLibraryTest COMMAND QGCUtilityLibraryTest)
set_tests_properties(UtilityLibraryTest PROPERTIES LABELS "Unit;Utilities" TIMEOUT 30)

foreach(target QGCIO QGCTiming QGCNetworkIO QGCLoggingCategory QGCJsonValidation)
    qgc_add_library_consumer(
        ${target}
        SOURCE
        "${CMAKE_CURRENT_SOURCE_DIR}/Consumers/${target}.cc"
        LABELS
        Unit
        Utilities
        TIMEOUT
        30
    )
endforeach()
