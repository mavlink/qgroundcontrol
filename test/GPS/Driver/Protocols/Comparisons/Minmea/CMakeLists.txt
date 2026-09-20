enable_language(C)
if(NOT COMMAND CPMAddPackage)
    include("${QGC_GPS_PROTOCOL_SOURCE_DIR}/../../../../cmake/modules/CPM.cmake")
endif()

set(_minmea_revision "04c13a272f5765680c38a812da8e82467b040ad8")
CPMAddPackage(
    NAME qgc_gps_minmea
    GITHUB_REPOSITORY kosma/minmea
    GIT_TAG "${_minmea_revision}"
    DOWNLOAD_ONLY YES
)

# Select the explicit MIT alternative; keep the grant and notice beside the comparison build.
configure_file("${qgc_gps_minmea_SOURCE_DIR}/LICENSE.grants" "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.grants" COPYONLY)
configure_file("${qgc_gps_minmea_SOURCE_DIR}/LICENSE.MIT" "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.MIT" COPYONLY)

add_library(QGCGPSMinmeaReference STATIC "${qgc_gps_minmea_SOURCE_DIR}/minmea.c")
target_compile_features(QGCGPSMinmeaReference PRIVATE c_std_99)
target_include_directories(QGCGPSMinmeaReference SYSTEM PUBLIC "${qgc_gps_minmea_SOURCE_DIR}")
set_target_properties(
    QGCGPSMinmeaReference
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
)
if(MSVC)
    configure_file("${qgc_gps_minmea_SOURCE_DIR}/compat/minmea_compat_windows.h"
                   "${CMAKE_CURRENT_BINARY_DIR}/minmea_compat.h" COPYONLY
    )
    target_include_directories(QGCGPSMinmeaReference PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    target_compile_definitions(QGCGPSMinmeaReference PRIVATE MINMEA_INCLUDE_COMPAT HAVE_STRUCT_TIMESPEC)
else()
    target_compile_definitions(QGCGPSMinmeaReference PRIVATE _POSIX_C_SOURCE=200809L _DEFAULT_SOURCE _DARWIN_C_SOURCE)
endif()
if(COMMAND qgc_disable_dependency_warnings)
    qgc_disable_dependency_warnings(QGCGPSMinmeaReference)
endif()
if(QGC_BUILD_GPS_PROTOCOL_FUZZER)
    target_compile_options(QGCGPSMinmeaReference PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(QGCGPSMinmeaReference INTERFACE -fsanitize=address,undefined)
endif()

target_sources(${CMAKE_PROJECT_NAME} PRIVATE MinmeaComparison.cc)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSMinmeaReference)
set_property(
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/MinmeaComparison.cc" TARGET_DIRECTORY ${CMAKE_PROJECT_NAME}
    APPEND
    PROPERTY COMPILE_DEFINITIONS GPS_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../fixtures"
             MINMEA_REFERENCE_REVISION="${_minmea_revision}"
)
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/MinmeaComparison.cc" TARGET_DIRECTORY ${CMAKE_PROJECT_NAME}
                            PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
)
add_qgc_test(GPSMinmeaComparisonTest LABELS Unit GPS Comparison)
