find_package(Qt6 REQUIRED COMPONENTS Core Network Quick)

target_sources(QGC
    PRIVATE
        ESP8266Component.cc
        ESP8266Component.h
        ESP8266ComponentController.cc
        ESP8266ComponentController.h
        MotorComponent.cc
        MotorComponent.h
        RadioComponentController.cc
        RadioComponentController.h
        SyslinkComponent.cc
        SyslinkComponent.h
        SyslinkComponentController.cc
        SyslinkComponentController.h
)

target_link_libraries(QGC
    PUBLIC
        Qt6::Core
        Qt6::Network
        Qt6::Quick
)

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

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
