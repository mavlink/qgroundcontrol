# ============================================================================
# Wire Utility Unit Tests
# Tests for WireFields (table-driven little-endian records)
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME} PRIVATE WireFieldsTest.cc WireFieldsTest.h)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCWire)

add_qgc_test(WireFieldsTest LABELS Unit Utilities)
