# ============================================================================
# Gimbal Module
# Camera gimbal control and stabilization
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        Gimbal.cc
        Gimbal.h
        GimbalController.cc
        GimbalController.h
)

# ----------------------------------------------------------------------------
# Gimbal Configuration Resources
# ----------------------------------------------------------------------------
qt_add_resources(${CMAKE_PROJECT_NAME} json_gimbal
    PREFIX "/json/Vehicle"
    FILES GimbalFact.json
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
