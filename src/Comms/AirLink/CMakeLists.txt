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

# qt_add_qml_module(QGC
#     URI QGroundControl.AirLink
#     VERSION 1.0
#     QML_FILES
#         AirLinkSettings.qml
#     OUTPUT_TARGETS AirLink_targets
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
#     IMPORTS
#         QtQuick
#         QtQuick.Controls
#         QtQuick.Layouts

#         QGroundControl
#         QGroundControl.Controllers
#         QGroundControl.Controls
#         QGroundControl.FactControls
#         QGroundControl.FactSystem
#         QGroundControl.ScreenTools
#         QGroundControl.SettingsManager
# )

# cmake_print_variables(AirLink_targets)
# target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE AirLinkplugin)
