# ============================================================================
# Network Utilities Unit Tests
# Tests for networking helper functions
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        QGCNetworkHelperTest.cc
        QGCNetworkHelperTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
