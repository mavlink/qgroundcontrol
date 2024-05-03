find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(AirLink STATIC)

option(QGC_AIRLINK_DISABLED "Enable airlink" ON)
if(NOT QGC_AIRLINK_DISABLED)
    find_package(Qt6 COMPONENTS Network REQUIRED)

    target_sources(AirLink
        PRIVATE
            AirlinkLink.cc
            AirlinkLink.h
            AirLinkManager.cc
            AirLinkManager.h
    )

    add_custom_target(AirLinkQml
        SOURCES
            AirLinkSettings.qml
    )

    target_link_libraries(AirLink
        PRIVATE
            Qt6::Network
            Settings
        PUBLIC
            Qt6::Core
            Comms
            QGC
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
    #         QtMultimedia
    #         QtQuick
    #         QtQuick.Controls
    #         QtQuick.Dialogs
    #         QtQuick.Layouts
    #         QtLocation
    #         QtPositioning

    #         QGroundControl
    #         QGroundControl.Controllers
    #         QGroundControl.Controls
    #         QGroundControl.FactControls
    #         QGroundControl.FactSystem
    #         QGroundControl.Palette
    #         QGroundControl.ScreenTools
    #         QGroundControl.SettingsManager
    # )
else()
    target_compile_definitions(AirLink PUBLIC QGC_AIRLINK_DISABLED)
endif()
