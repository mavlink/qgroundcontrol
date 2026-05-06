# ============================================================================
# PX4ULog Utility Unit Tests
# Tests for PX4ULogUtility (PX4 ULog parsing)
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        PX4ULogUtilityTest.cc
        PX4ULogUtilityTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
