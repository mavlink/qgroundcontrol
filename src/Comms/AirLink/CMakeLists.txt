if(QGC_AIRLINK_DISABLED)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_AIRLINK_DISABLED)
    return()
endif()

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        AirLinkLink.cc
        AirLinkLink.h
        AirLinkManager.cc
        AirLinkManager.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_library(AirLinkModule STATIC)

qt_add_qml_module(AirLinkModule
    URI QGroundControl.AirLink
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        AirLinkSettings.qml
    NO_PLUGIN
)
