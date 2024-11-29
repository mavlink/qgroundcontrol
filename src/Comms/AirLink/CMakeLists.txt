find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(AirLink STATIC)

if(NOT QGC_AIRLINK_DISABLED)
    find_package(Qt6 REQUIRED COMPONENTS Network QmlIntegration)

    target_sources(AirLink
        PRIVATE
            AirLinkLink.cc
            AirLinkLink.h
            AirLinkManager.cc
            AirLinkManager.h
    )

    target_link_libraries(AirLink
        PRIVATE
            Qt6::Network
            Settings
            Utilities
        PUBLIC
            Qt6::Core
            Qt6::QmlIntegration
            Comms
    )

    target_include_directories(AirLink PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

    # qt_add_qml_module(AirLink
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
    # target_link_libraries(AirLink PRIVATE AirLinkplugin)
else()
    target_compile_definitions(AirLink PUBLIC QGC_AIRLINK_DISABLED)
endif()
