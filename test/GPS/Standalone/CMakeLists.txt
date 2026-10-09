# ============================================================================
# GPS Standalone
# QGCGPSCore and QGCNTRIPHttp without the application, with the receiver qualification runner, the NTRIP decoder smoke
# test and, with Clang and QGC_GPS_LIBFUZZER, the libFuzzer harnesses:
#   cmake -S test/GPS/Standalone -B <build> [-DQGC_NO_SERIAL_LINK=ON] [-DQGC_GPS_LIBFUZZER=ON]
# ============================================================================

cmake_minimum_required(VERSION 3.25)
project(QGCGPSStandalone LANGUAGES CXX)
enable_testing()

get_filename_component(QGC_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
file(READ "${QGC_SOURCE_ROOT}/.github/build-config.json" _qgc_build_config)
string(JSON _qgc_qt_minimum_version GET "${_qgc_build_config}" qt minimum_version)

option(QGC_NO_SERIAL_LINK "Build without serial receiver support" OFF)
option(QGC_GPS_LIBFUZZER
       "Build the libFuzzer harnesses, with the GPS libraries and their consumers under ASan and UBSan" OFF
)
set(_qgc_qt_components Core Network)
if(NOT QGC_NO_SERIAL_LINK)
    list(APPEND _qgc_qt_components SerialPort)
endif()
find_package(Qt6 ${_qgc_qt_minimum_version} REQUIRED COMPONENTS ${_qgc_qt_components})

# The GPS CMakeLists name these utility targets and rely on the application build to define them.
foreach(
    directory
    Utilities/Logging/Category
    Utilities/Math
    Utilities/Timing
    Utilities/Network/Client
    Utilities/Parsing/Wire
    GPS/Core
    GPS/NTRIP/HTTP
)
    string(REPLACE "/" "" _qgc_binary_directory "${directory}")
    add_subdirectory("${QGC_SOURCE_ROOT}/src/${directory}" "qgc/${_qgc_binary_directory}")
endforeach()
# A QGC library missing from the list above would otherwise surface only as a link error.
foreach(target QGCGPSCore QGCNTRIPHttp)
    get_target_property(_qgc_dependencies ${target} LINK_LIBRARIES)
    foreach(dependency IN LISTS _qgc_dependencies)
        if(dependency MATCHES "^QGC" AND NOT TARGET ${dependency})
            message(FATAL_ERROR "${target} needs ${dependency}; add its directory to test/GPS/Standalone")
        endif()
    endforeach()
endforeach()

if(QGC_GPS_LIBFUZZER)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR MSVC)
        message(FATAL_ERROR "QGC_GPS_LIBFUZZER requires a Clang driver with libFuzzer and sanitizer runtimes")
    endif()
    foreach(target QGCGPSCore QGCNTRIPHttp)
        target_compile_options(${target} PUBLIC -fsanitize=fuzzer-no-link,address,undefined -fno-omit-frame-pointer)
        target_link_options(${target} INTERFACE -fsanitize=address,undefined)
    endforeach()
endif()

add_subdirectory(../Core/Transport/Support TestTransports)
add_subdirectory(../Core/Protocols/Support ProtocolTestSupport)
add_subdirectory(../Core/Protocols/Hardware Hardware)
add_subdirectory(../NTRIP/HTTP/Fuzz NTRIPHttpFuzz)
if(QGC_GPS_LIBFUZZER)
    add_subdirectory(../Core/Protocols/Fuzz ProtocolFuzz)
endif()
