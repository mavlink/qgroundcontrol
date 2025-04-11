set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/AutoPilotPlugin.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/AutoPilotPlugin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Generic/GenericAutoPilotPlugin.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/Generic/GenericAutoPilotPlugin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/VehicleComponent.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/VehicleComponent.h
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Generic
    CACHE INTERNAL "" FORCE
)

add_subdirectory(Common)
if(NOT QGC_DISABLE_APM_PLUGIN)
    add_subdirectory(APM)
endif()
if(NOT QGC_DISABLE_PX4_PLUGIN)
    add_subdirectory(PX4)
endif()
