# ============================================================================
# AutoPilot Plugins Module
# Provides autopilot-specific UI and configuration handling
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        AutoPilotPlugin.cc
        AutoPilotPlugin.h
        Generic/GenericAutoPilotPlugin.cc
        Generic/GenericAutoPilotPlugin.h
        VehicleComponent.cc
        VehicleComponent.h
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        Generic
)

# ----------------------------------------------------------------------------
# Autopilot-Specific Plugins
# ----------------------------------------------------------------------------
add_subdirectory(Common)
add_subdirectory(APM)
add_subdirectory(PX4)
