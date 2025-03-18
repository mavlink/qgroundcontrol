if(NOT QGC_DISABLE_APM_PLUGIN)
    add_subdirectory(APM)
endif()
if(NOT QGC_DISABLE_PX4_PLUGIN)
    add_subdirectory(PX4)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core)

target_sources(QGC
    PRIVATE
        AutoPilotPlugin.cc
        AutoPilotPlugin.h
        Generic/GenericAutoPilotPlugin.cc
        Generic/GenericAutoPilotPlugin.h
        VehicleComponent.cc
        VehicleComponent.h
)

target_link_libraries(QGC
    PUBLIC
        Qt6::Core
)

target_include_directories(QGC
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        Generic
)

add_subdirectory(Common)
