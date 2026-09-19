include_guard(GLOBAL)

if(NOT TARGET QGCGPSPx4Adapter)
    add_library(QGCGPSPx4Adapter STATIC GPSPx4Data.cc GPSPx4Data_p.h satellite_info.h sensor_gps.h)
    target_compile_features(QGCGPSPx4Adapter PRIVATE cxx_std_20)
    set_target_properties(
        QGCGPSPx4Adapter
        PROPERTIES AUTOMOC OFF
                   AUTOUIC OFF
                   AUTORCC OFF
                   POSITION_INDEPENDENT_CODE ON
    )
    target_include_directories(QGCGPSPx4Adapter PRIVATE ../../Receiver)
    target_link_libraries(QGCGPSPx4Adapter PRIVATE QGCGPSDriverReports)
endif()

# Only the runtime facade opts into Qt and the external protocol implementation.
function(qgc_add_px4_backend target)
    if(NOT ARGC EQUAL 1 OR NOT TARGET "${target}")
        message(FATAL_ERROR "qgc_add_px4_backend requires one existing driver target")
    endif()
    if(TARGET px4-gpsdrivers)
        message(FATAL_ERROR "The PX4 GPS backend is already configured")
    endif()

    include(CPMPatchCache)
    set(px4_gpsdrivers_tag cd6f506afda9bd6e8e4645f094dcf5415f1f8be4)
    set(px4_gpsdrivers_patches "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/ubx-position-time-mode.patch"
                               "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/non-ubx-safety.patch"
    )
    qgc_cpm_patch_cache_key(
        px4_gpsdrivers_cache_key
        GIT_REPOSITORY
        "https://github.com/PX4/PX4-GPSDrivers.git"
        GIT_TAG
        "${px4_gpsdrivers_tag}"
        PATCHES
        ${px4_gpsdrivers_patches}
    )
    CPMAddPackage(
        NAME px4-gpsdrivers
        GITHUB_REPOSITORY PX4/PX4-GPSDrivers
        GIT_TAG "${px4_gpsdrivers_tag}"
        SOURCE_SUBDIR src
        CUSTOM_CACHE_KEY "${px4_gpsdrivers_cache_key}"
        PATCHES ${px4_gpsdrivers_patches}
    )

    file(GLOB gps_drivers_sources CONFIGURE_DEPENDS "${px4-gpsdrivers_SOURCE_DIR}/src/*.c"
         "${px4-gpsdrivers_SOURCE_DIR}/src/*.cpp" "${px4-gpsdrivers_SOURCE_DIR}/src/*.h"
    )
    add_library(px4-gpsdrivers OBJECT ${gps_drivers_sources})
    target_include_directories(px4-gpsdrivers SYSTEM PRIVATE "${px4-gpsdrivers_SOURCE_DIR}/src")
    target_include_directories(px4-gpsdrivers PRIVATE "${CMAKE_CURRENT_FUNCTION_LIST_DIR}")
    target_compile_definitions(px4-gpsdrivers
                               PRIVATE GPS_DEFINITIONS_HEADER=<${CMAKE_CURRENT_FUNCTION_LIST_DIR}/definitions.h>
    )
    target_link_libraries(px4-gpsdrivers PRIVATE Qt6::Core)
    qgc_disable_dependency_warnings(px4-gpsdrivers)

    target_sources(${target} PRIVATE "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/definitions.h"
                                     "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/sensor_gnss_relative.h"
    )
    target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_FUNCTION_LIST_DIR}")
    target_include_directories(${target} SYSTEM PRIVATE "${px4-gpsdrivers_SOURCE_DIR}/src")
    target_compile_definitions(${target}
                               PRIVATE GPS_DEFINITIONS_HEADER=<${CMAKE_CURRENT_FUNCTION_LIST_DIR}/definitions.h>
    )
    target_link_libraries(${target} PRIVATE QGCGPSPx4Adapter px4-gpsdrivers)
endfunction()
