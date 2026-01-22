# ============================================================================
# Joystick Unit Tests
# Tests for joystick functionality using SDL virtual joysticks
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        JoystickTest.cc
        JoystickTest.h
        JoystickManagerTest.cc
        JoystickManagerTest.h
        MockJoystick.cc
        MockJoystick.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
