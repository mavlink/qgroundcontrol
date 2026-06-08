# ============================================================================
# Communications Unit Tests
# Tests for communication links; protocol- and transport-specific tests live in subdirectories
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        LinkConfigurationTest.cc
        LinkConfigurationTest.h
        LinkManagerTest.cc
        LinkManagerTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(Bluetooth)
add_subdirectory(Serial)

add_qgc_test(LinkConfigurationTest LABELS Unit Comms RESOURCE_LOCK Settings TempFiles)
add_qgc_test(LinkManagerTest LABELS Integration Comms SERIAL)
