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

add_subdirectory(Common)
if(NOT QGC_DISABLE_APM_PLUGIN)
    add_subdirectory(APM)
endif()
if(NOT QGC_DISABLE_PX4_PLUGIN)
    add_subdirectory(PX4)
endif()
