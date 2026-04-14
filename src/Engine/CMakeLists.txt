# ===================================================================
# Engine Status Subsystem
# ===================================================================
target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        Engine.cc
        Engine.h
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

QT_ADD_LIBRARY(EngineStatusModule STATIC)

QT_ADD_QML_MODULE(EngineStatusModule
    URI QGroundControl.EngineStatus
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        EngineStatus.qml
        EngineSummary.qml
    NO_PLUGIN
)
