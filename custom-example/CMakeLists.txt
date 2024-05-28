qt_add_library(CustomPlugin STATIC
	src/CustomPlugin.cc
	src/CustomPlugin.h
	src/AutoPilotPlugin/CustomAutoPilotPlugin.cc
	src/AutoPilotPlugin/CustomAutoPilotPlugin.h
	src/Facts/CustomFactGroup.cc
	src/Facts/CustomFactGroup.h
	src/FirmwarePlugin/CustomFirmwarePlugin.cc
	src/FirmwarePlugin/CustomFirmwarePlugin.h
	src/Settings/CustomSettings.cc
	src/Settings/CustomSettings.h
	custom.qrc
	InstrumentValueIcons.qrc
	qgcresources.qrc
	qgroundcontrol.qrc
)

target_sources(FirmwarePlugin
	PRIVATE
		src/FirmwarePlugin/CustomFirmwarePluginFactory.cc
		src/FirmwarePlugin/CustomFirmwarePluginFactory.h
)

target_link_libraries(CustomPlugin
	PRIVATE
		QGC
	PUBLIC
		API
		FactSystem
		FirmwarePlugin
		Settings
		Utilities
		Vehicle
)

target_include_directories(CustomPlugin
	PUBLIC
		src
		src/AutoPilotPlugin
		src/Facts
		src/FirmwarePlugin
		src/Settings
)

target_compile_definitions(CustomPlugin
	PUBLIC
		QGC_CUSTOM_BUILD
		CUSTOMHEADER="CustomPlugin.h"
		CUSTOMCLASS=CustomPlugin
)

# add_compile_definitions(
#     QGC_APPLICATION_NAME="QGroundControl"
#     QGC_ORG_NAME="QGroundControl.org"
#     QGC_ORG_DOMAIN="org.qgroundcontrol"
#     APP_VERSION_STR="${APP_VERSION_STR}"
# )

set_source_files_properties(src/Facts/CustomFact.json
    PROPERTIES
        QT_RESOURCE_ALIAS CustomFact.json
)

set_source_files_properties(src/Settings/Custom.SettingsGroup.json
    PROPERTIES
        QT_RESOURCE_ALIAS Custom.SettingsGroup.json
)

qt_add_resources(CustomPlugin "CustomPluginResources"
	FILES
		src/Facts/CustomFact.json
		src/Settings/Custom.SettingsGroup.json
)

set_source_files_properties(res/CustomFlyViewOverlay.qml
    PROPERTIES
        QT_RESOURCE_ALIAS CustomFlyViewOverlay.qml
)

file(GLOB_RECURSE QML_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/res/*.qml)
file(GLOB QML_RESOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/res/Images/*.*)
qt_add_qml_module(CustomPlugin
    URI QGroundControl.CustomPlugin
    VERSION 1.0
    RESOURCE_PREFIX "/"
    QML_FILES ${QML_SOURCES}
    RESOURCES ${QML_RESOURCES}
    OUTPUT_TARGETS CustomPlugin_targets
    IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
    IMPORTS
		QtQuick
		QtQuick.Controls
		QtQuick.Layouts
		QGroundControl
		QGroundControl.Controls
		QGroundControl.Palette
		QGroundControl.ScreenTools
)

target_link_libraries(${PROJECT_NAME} PRIVATE CustomPlugin)

if(WIN32)
    set_target_properties(${PROJECT_NAME}
        PROPERTIES
            QT_TARGET_RC_ICONS res/Images/CustomWindows.ico
    )
elseif(MACOS)
    # set_target_properties(${PROJECT_NAME}
    #     PROPERTIES
    #         MACOSX_BUNDLE TRUE
    #         MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/deploy/mac/MacOSXBundleInfo.plist.in"
    #         MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}"
    #         MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    #         MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 2020 QGroundControl. All rights reserved."
    #         MACOSX_BUNDLE_GUI_IDENTIFIER "io.mavlink.qgroundcontrol"
    #         MACOSX_BUNDLE_ICON_FILE "macx.icns"
    #         MACOSX_BUNDLE_INFO_STRING "Open Source Ground Control App"
    #         MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
    #         MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
    # )
elseif(ANDROID)
	get_target_property(PACKAGE_SOURCE_DIR ${PROJECT_NAME} QT_ANDROID_PACKAGE_SOURCE_DIR)
	file(COPY android/* DESTINATION ${PACKAGE_SOURCE_DIR}/)

	set_target_properties(${PROJECT_NAME}
        PROPERTIES
            QT_ANDROID_ABIS "arm64-v8a"
            QT_ANDROID_MIN_SDK_VERSION 23
            QT_ANDROID_TARGET_SDK_VERSION 33
    )
elseif(LINUX)
    # install(
    #     FILES ${CMAKE_SOURCE_DIR}/deploy/linux/org.mavlink.qgroundcontrol.desktop
    #     DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    # )
    # install(
    #     FILES ${CMAKE_SOURCE_DIR}/resources/icons/qgroundcontrol.png
    #     DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps/
    #     RENAME org.mavlink.qgroundcontrol.png
    # )
    # configure_file(
    #     ${CMAKE_SOURCE_DIR}/deploy/linux/org.mavlink.qgroundcontrol.metainfo.xml.in
    #     ${CMAKE_BINARY_DIR}/metainfo/org.mavlink.qgroundcontrol.metainfo.xml
    #     @ONLY
    # )
    # install(
    #     FILES ${CMAKE_BINARY_DIR}/metainfo/org.mavlink.qgroundcontrol.metainfo.xml
    #     DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo/
    # )
endif()
