# ============================================================================
# Performance Benchmark Tests
# Measures performance of key operations
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        PerformanceBenchmarkTest.cc PerformanceBenchmarkTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
