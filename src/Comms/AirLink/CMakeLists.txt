if(QGC_AIRLINK_DISABLED)
    set(QGC_DEFINES
        ${QGC_DEFINES}
        QGC_AIRLINK_DISABLED
        CACHE INTERNAL "" FORCE
    )
    return()
endif()

set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/AirLinkLink.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/AirLinkLink.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AirLinkManager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/AirLinkManager.h
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

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

# target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE AirLinkplugin)
