get_filename_component(QGC_FIXPOSITION_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../.." ABSOLUTE)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(FATAL_ERROR "The Fixposition frame-only prototype currently requires a Linux GCC/Clang linker")
endif()
if(NOT COMMAND CPMAddPackage)
    include("${QGC_FIXPOSITION_ROOT}/cmake/modules/CPM.cmake")
endif()

# DOWNLOAD_ONLY deliberately avoids the Linux-oriented SDK monolith and its optional dependencies.
CPMAddPackage(
    NAME qgc_fixposition_sdk
    GITHUB_REPOSITORY fixposition/fixposition-sdk
    GIT_TAG 200d20167551da4c1bb57b7e0eb2331e8f070f36
    DOWNLOAD_ONLY YES
)

# rtcm3.cpp includes trafo.hpp, which needs Eigen Core/Geometry declarations even for framing-only use.
# No Eigen/SDK solver, transform implementation, ROS, video, Boost, or SDK CMake target is built.
CPMAddPackage(
    NAME qgc_fixposition_eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG 3147391d946bb4b6c68edd901f2add6ac1f31f8c
    DOWNLOAD_ONLY YES
)

set(_fpsdk_common "${qgc_fixposition_sdk_SOURCE_DIR}/fpsdk_common")
set(_fpsdk_sources "${_fpsdk_common}/src/parser.cpp")
foreach(
    part
    crc
    fpa
    fpb
    nmea
    novb
    qgc
    rtcm3
    sbf
    spartn
    types
    ubx
    unib
)
    list(APPEND _fpsdk_sources "${_fpsdk_common}/src/parser/${part}.cpp")
endforeach()
add_library(QGCGPSFixpositionFrames OBJECT ${_fpsdk_sources})
target_compile_features(QGCGPSFixpositionFrames PRIVATE cxx_std_20)
target_include_directories(QGCGPSFixpositionFrames SYSTEM PUBLIC "${_fpsdk_common}/include")
target_include_directories(QGCGPSFixpositionFrames SYSTEM PRIVATE "${qgc_fixposition_eigen_SOURCE_DIR}")
target_compile_definitions(QGCGPSFixpositionFrames PRIVATE EIGEN_MPL2_ONLY)
# Keep the actual upstream framing/naming/checksum closure, not unused semantic/debug/utility entry points.
# The linker must reject missing reachable implementations; there are no SDK replacement functions.
target_compile_options(QGCGPSFixpositionFrames PRIVATE -ffunction-sections -fdata-sections)
set_target_properties(
    QGCGPSFixpositionFrames
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/licenses")
configure_file("${_fpsdk_common}/LICENSE" "${CMAKE_CURRENT_BINARY_DIR}/licenses/Fixposition-LICENSE" COPYONLY)
configure_file("${qgc_fixposition_eigen_SOURCE_DIR}/COPYING.MPL2"
               "${CMAKE_CURRENT_BINARY_DIR}/licenses/Eigen-COPYING.MPL2" COPYONLY
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE FixpositionComparison.cc)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSFixpositionFrames)
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE "LINKER:--gc-sections")
set_property(
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/FixpositionComparison.cc" TARGET_DIRECTORY ${CMAKE_PROJECT_NAME}
    APPEND
    PROPERTY COMPILE_DEFINITIONS GPS_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../fixtures"
             GPS_CORPUS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../corpus"
)
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/FixpositionComparison.cc" TARGET_DIRECTORY
                            ${CMAKE_PROJECT_NAME} PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
)

if(COMMAND qgc_apply_sanitizers_to_target)
    qgc_apply_sanitizers_to_target(QGCGPSFixpositionFrames)
endif()
if(QGC_BUILD_GPS_PROTOCOL_FUZZER)
    # The SDK object target does not inherit the production framers' sanitizer usage requirements.
    target_compile_options(QGCGPSFixpositionFrames PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
endif()

# Report-only verifies harness integrity, byte conservation, fragmentation and bounds, not candidate compatibility.
# QGC_GPS_FIXPOSITION_REQUIRE_COMPATIBLE=1 enables the intentionally strict adoption gate.
add_qgc_test(
    GPSFixpositionComparisonTest
    LABELS
    Unit
    GPS
    Comparison
    Prototype
    TIMEOUT
    120
)
