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
        MockJoystickTest.cc
        MockJoystickTest.h
        SDLTest.cc
        SDLTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(JoystickTest LABELS Unit Joystick RESOURCE_LOCK Joystick)
add_qgc_test(JoystickManagerTest LABELS Unit Joystick RESOURCE_LOCK Joystick TIMEOUT ${QGC_TEST_TIMEOUT_EXTENDED})
add_qgc_test(MockJoystickTest LABELS Unit Joystick RESOURCE_LOCK Joystick)
add_qgc_test(SDLTest LABELS Unit Joystick RESOURCE_LOCK Joystick)
