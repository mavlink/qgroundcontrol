include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/../../../src/GPS/cmake/GPSBuild.cmake")

# Logging registry injection is local to the harness; all production sources and dependencies belong to GPSBuild.
function(qgc_add_gps_parser_runtime target)
    get_filename_component(gps_repository "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../../.." ABSOLUTE)
    add_library(${target}Logging STATIC "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/GPSParserLogging.cc")
    target_include_directories(${target}Logging PUBLIC "${gps_repository}/src/Utilities/Logging")
    target_link_libraries(${target}Logging PUBLIC Qt6::Core)
    set(components Contracts Scheduler Core NMEA NTRIPSession Corrections)
    set(component_targets ${target}Logging)
    foreach(component IN LISTS components)
        qgc_add_gps_component(
            ${target}${component}
            COMPONENT
            ${component}
            PREFIX
            ${target}
            LOGGING_TARGET
            ${target}Logging
        )
        list(APPEND component_targets ${target}${component})
    endforeach()
    add_library(${target} INTERFACE)
    target_link_libraries(${target} INTERFACE ${component_targets})
    set_property(TARGET ${target} PROPERTY QGC_GPS_COMPONENT_TARGETS "${component_targets}")
endfunction()

# Only the native ABI clock differs between the application and deterministic replay.
function(qgc_add_gps_replay_driver target parser_target)
    get_filename_component(gps_repository "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../../.." ABSOLUTE)
    if(NOT TARGET QGCJsonValidation)
        add_subdirectory("${gps_repository}/src/Utilities/Parsing/Json" JsonValidation)
    endif()
    foreach(
        component
        Native
        Transport
        Driver
        Recording
        ReceiverTransports
        Receiver
        RecordingController
    )
        qgc_add_gps_component(
            ${parser_target}${component}
            COMPONENT
            ${component}
            PREFIX
            ${parser_target}
            LOGGING_TARGET
            ${parser_target}Logging
            NATIVE_DEFINITIONS_HEADER
            "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/GPSReplayDefinitions.h"
        )
    endforeach()
    add_library(${target} INTERFACE)
    target_link_libraries(${target} INTERFACE ${parser_target} ${parser_target}Driver ${parser_target}Recording
                                              ${parser_target}Receiver ${parser_target}RecordingController
    )
    target_include_directories(${target} INTERFACE "${gps_repository}/src/GPS/Driver/PX4")
    target_compile_definitions(
        ${target} INTERFACE GPS_DEFINITIONS_HEADER="${CMAKE_CURRENT_FUNCTION_LIST_DIR}/GPSReplayDefinitions.h"
    )
endfunction()
