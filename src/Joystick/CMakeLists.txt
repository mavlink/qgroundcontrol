# ============================================================================
# Joystick Module
# Game controller and joystick input support
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        Joystick.cc
        Joystick.h
        JoystickManager.cc
        JoystickManager.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# SDL Joystick Implementation (All Platforms)
# ----------------------------------------------------------------------------
target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        JoystickSDL.cc
        JoystickSDL.h
)
