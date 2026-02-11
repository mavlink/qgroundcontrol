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
        logging.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ============================================================================
# LibEvents Library Integration
# ============================================================================

CPMAddPackage(
    NAME libevents
    GITHUB_REPOSITORY mavlink/libevents
    GIT_TAG main
    SOURCE_SUBDIR libs/cpp
)

# libevents is a header-only (INTERFACE) library, so we cannot suppress warnings
# on its target. Instead, mark its include directories as SYSTEM to suppress
# warnings when its headers are compiled as part of QGC source code.
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE libevents)
target_include_directories(${CMAKE_PROJECT_NAME} SYSTEM PRIVATE ${libevents_SOURCE_DIR}/libs/cpp)
