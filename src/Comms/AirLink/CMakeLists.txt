find_package(Qt6 REQUIRED COMPONENTS Core)

if(NOT QGC_AIRLINK_DISABLED)
    find_package(Qt6 REQUIRED COMPONENTS Network QmlIntegration)

    target_sources(QGC
        PRIVATE
            AirLinkLink.cc
            AirLinkLink.h
            AirLinkManager.cc
            AirLinkManager.h
    )

    target_link_libraries(QGC
        PRIVATE
            Qt6::Network
        PUBLIC
            Qt6::Core
            Qt6::QmlIntegration
    )

    target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

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
    # target_link_libraries(QGC PRIVATE AirLinkplugin)
else()
    target_compile_definitions(QGC PUBLIC QGC_AIRLINK_DISABLED)
endif()
