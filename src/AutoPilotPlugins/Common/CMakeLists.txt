set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/ESP8266Component.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ESP8266Component.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ESP8266ComponentController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ESP8266ComponentController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MotorComponent.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MotorComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/RadioComponentController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/RadioComponentController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SyslinkComponent.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/SyslinkComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SyslinkComponentController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/SyslinkComponentController.h
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

# file(GLOB QML_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.qml)
# file(GLOB QML_RESOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Images/*.*)
# qt_add_qml_module(QGC
#     URI QGroundControl.CommonAutoPilotPlugin
#     VERSION 1.0
#     QML_FILES ${QML_SOURCES}
#     RESOURCES ${QML_RESOURCES}
#     OUTPUT_TARGETS CommonAutoPilotPlugin_targets
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
#     IMPORTS
#         QtQuick
#         QtQuick.Controls
#         QtQuick.Dialogs
#         QtQuick.Layouts

#         QGroundControl
#         QGroundControl.FactSystem
#         QGroundControl.FactControls
#         QGroundControl.Controls
#         QGroundControl.ScreenTools
#         QGroundControl.Controllers
#         QGroundControl.Palette
# )
