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

set(_libevents_patch "${CMAKE_CURRENT_LIST_DIR}/libevents-sysid32.patch")
qgc_cpm_patch_cache_key(
    _libevents_cache_key
    GIT_REPOSITORY
    "https://github.com/mavlink/libevents.git"
    GIT_TAG
    840a88ea226d4eb0fd4c391ce860317422756435
    PATCHES
    "${_libevents_patch}"
)

CPMAddPackage(
    NAME libevents
    GITHUB_REPOSITORY mavlink/libevents
    GIT_TAG 840a88ea226d4eb0fd4c391ce860317422756435
    CUSTOM_CACHE_KEY "${_libevents_cache_key}"
    PATCHES "${_libevents_patch}"
    SOURCE_SUBDIR libs/cpp
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE libevents)
target_include_directories(${CMAKE_PROJECT_NAME} SYSTEM PRIVATE ${libevents_SOURCE_DIR}/libs/cpp)
