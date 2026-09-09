cmake_minimum_required(VERSION 3.25)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(QGCPX4DriverTests LANGUAGES CXX)
    enable_testing()
endif()

# Compile the checked-in driver bytes with the host test platform contract. These
# tests need neither a Qt installation nor the separate PX4-GPSDrivers checkout.
get_filename_component(QGC_PX4_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/GPS/Driver/PX4" ABSOLUTE)

# Keep native tests independent of Qt autogen and application compile definitions.
function(qgc_add_px4_test_target target)
    add_executable(${target} ${ARGN})
    target_compile_features(${target} PRIVATE cxx_std_20)
    target_include_directories(${target} PRIVATE "${QGC_PX4_SOURCE_DIR}")
    target_compile_definitions(${target} PRIVATE GPS_DEFINITIONS_HEADER="${CMAKE_CURRENT_SOURCE_DIR}/definitions.h")
    set_target_properties(
        ${target}
        PROPERTIES AUTOMOC OFF
                   AUTOUIC OFF
                   AUTORCC OFF
    )
    if(MSVC)
        target_compile_options(${target} PRIVATE /W3 /EHsc)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    endif()
endfunction()

qgc_add_px4_test_target(QGCPX4ParserTest gps-parser-test.cpp "${QGC_PX4_SOURCE_DIR}/unicore.cpp"
                        "${QGC_PX4_SOURCE_DIR}/crc.cpp"
)
qgc_add_px4_test_target(QGCPX4UtcTest gps-time-test.cpp)
qgc_add_px4_test_target(QGCPX4UtcFallbackTest gps-time-test.cpp)
target_compile_definitions(QGCPX4UtcFallbackTest PRIVATE GPS_NO_TIMEGM)
qgc_add_px4_test_target(QGCPX4UbxTest gps-ubx-test.cpp "${QGC_PX4_SOURCE_DIR}/ubx.cpp"
                        "${QGC_PX4_SOURCE_DIR}/gps_helper.cpp" "${QGC_PX4_SOURCE_DIR}/rtcm.cpp"
)
qgc_add_px4_test_target(
    QGCPX4ReceiverModesTest
    gps-receiver-mode-test.cpp
    "${QGC_PX4_SOURCE_DIR}/sbf.cpp"
    "${QGC_PX4_SOURCE_DIR}/femtomes.cpp"
    "${QGC_PX4_SOURCE_DIR}/gps_helper.cpp"
    "${QGC_PX4_SOURCE_DIR}/rtcm.cpp"
    "${QGC_PX4_SOURCE_DIR}/crc.cpp"
)

foreach(test IN ITEMS Parser Ubx ReceiverModes)
    add_test(NAME PX4GPS${test} COMMAND QGCPX4${test}Test)
    set_tests_properties(PX4GPS${test} PROPERTIES LABELS "Unit;GPS;PX4Driver" TIMEOUT 60)
endforeach()

foreach(timezone IN ITEMS UTC0 EST5EDT JST-9)
    foreach(variant IN ITEMS Utc UtcFallback)
        add_test(NAME PX4GPS${variant}-${timezone} COMMAND QGCPX4${variant}Test)
        set_tests_properties(PX4GPS${variant}-${timezone} PROPERTIES LABELS "Unit;GPS;PX4Driver" TIMEOUT 60 ENVIRONMENT
                                                                     "TZ=${timezone}"
        )
    endforeach()
endforeach()
