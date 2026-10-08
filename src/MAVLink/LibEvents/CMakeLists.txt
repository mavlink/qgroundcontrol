# ============================================================================
# LibEvents - MAVLink Event Handling
# Event logging, health reporting, and arming checks
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        EventHandler.cc
        EventHandler.h
        HealthAndArmingCheckReport.cc
        HealthAndArmingCheckReport.h
        LibEvents.cc
        LibEvents.h
        MAVLinkEventManager.cc
        MAVLinkEventManager.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ============================================================================
# LibEvents Library Integration
# ============================================================================

if(QGC_USE_SYSTEM_LIBS OR QGC_SYSTEM_LIBS_ONLY)
    # ALT ships libevents as separate libraries (parser + health checks).
    find_library(EVENTS_PARSER_LIB NAMES events_parser)
    find_library(EVENTS_HEALTH_LIB NAMES events_health_and_arming_checks)
    if(NOT EVENTS_PARSER_LIB OR NOT EVENTS_HEALTH_LIB)
        message(FATAL_ERROR "QGC: libevents (required dependency) was not resolved from system")
    endif()
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${EVENTS_PARSER_LIB} ${EVENTS_HEALTH_LIB})
    # QGC includes protocol/receive.h, parse/..., generated/... — the distro
    # libevents may not export that include root via its imported target.
    set(LIBEVENTS_SYSTEM_INCLUDE_DIR "/usr/include/libevents" CACHE PATH
        "libevents include root (contains protocol/, parse/, generated/)")
    target_include_directories(${CMAKE_PROJECT_NAME} SYSTEM PRIVATE "${LIBEVENTS_SYSTEM_INCLUDE_DIR}")
else()
    CPMAddPackage(
        NAME libevents
        GITHUB_REPOSITORY mavlink/libevents
        GIT_TAG 840a88ea226d4eb0fd4c391ce860317422756435
        SOURCE_SUBDIR libs/cpp
    )

    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE libevents)
    target_include_directories(${CMAKE_PROJECT_NAME} SYSTEM PRIVATE ${libevents_SOURCE_DIR}/libs/cpp)
endif()
