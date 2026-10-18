# ============================================================================
# LibEvents - MAVLink Event Handling
# Event logging, health reporting, and arming checks
# ============================================================================

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE EventHandler.cc
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

include(CPMPatchCache)

set(_libevents_repository "https://github.com/mavlink/libevents.git")
set(_libevents_tag 840a88ea226d4eb0fd4c391ce860317422756435)
# Exposes the event ID on HealthAndArmingChecks::Check so warning acknowledgements survive changing message values.
# Remove once upstream libevents provides it.
set(_libevents_patch "${CMAKE_CURRENT_LIST_DIR}/libevents-check-event-id.patch")
qgc_cpm_patch_cache_key(
    _libevents_cache_key
    GIT_REPOSITORY
    "${_libevents_repository}"
    GIT_TAG
    "${_libevents_tag}"
    PATCHES
    "${_libevents_patch}"
)

CPMAddPackage(
    NAME libevents
    GIT_REPOSITORY ${_libevents_repository}
    GIT_TAG ${_libevents_tag}
    SOURCE_SUBDIR libs/cpp
    # CPM's default source-cache key does not hash PATCHES file contents.
    CUSTOM_CACHE_KEY "${_libevents_cache_key}"
    PATCHES "${_libevents_patch}"
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE libevents)
target_include_directories(${CMAKE_PROJECT_NAME} SYSTEM PRIVATE ${libevents_SOURCE_DIR}/libs/cpp)
