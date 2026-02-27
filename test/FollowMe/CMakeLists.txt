# ============================================================================
# Follow Me Unit Tests
# Tests for Follow Me mode and vehicle tracking
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        FollowMeTest.cc
        FollowMeTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
