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

qt_add_library(AutoPilotPluginsModule STATIC)

qt_add_qml_module(AutoPilotPluginsModule
    URI QGroundControl.AutoPilotPlugins
    VERSION 1.0
    RESOURCE_PREFIX /qml
    NO_PLUGIN # Required by Qt 6.6.3. TODO: Remove when no longer supported.
)

add_subdirectory(Common)
if(NOT QGC_DISABLE_APM_PLUGIN)
    add_subdirectory(APM)
endif()
if(NOT QGC_DISABLE_PX4_PLUGIN)
    add_subdirectory(PX4)
endif()
