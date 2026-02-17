# ============================================================================
# ULog Utility Unit Tests
# Tests for ULogUtility (general-purpose ULog parsing)
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ULogUtilityTest.cc
        ULogUtilityTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
