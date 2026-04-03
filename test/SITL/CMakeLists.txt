# ============================================================================
# SITL Integration Tests
# ============================================================================
# Tests that run QGC against a real PX4 SITL instance via Docker.
# Disabled by default — enable with -DQGC_SITL_TESTS=ON.
# Requires Docker and the px4io/px4-sitl-sih container image.

if(NOT QGC_SITL_TESTS)
    return()
endif()

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        SITLTestBase.h
        SITLTestBase.cc
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(MAVLink)
add_subdirectory(PX4)
