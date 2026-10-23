# ---------------------------------------------------------------------------
# Code-generation: produce QML settings pages from JSON definitions
# ---------------------------------------------------------------------------

set(SETTINGS_PAGES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/pages")
set(SETTINGS_METADATA_DIR "${CMAKE_SOURCE_DIR}/src/Settings")
set(SETTINGS_QML_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

# JSON inputs: page definitions + settings metadata
file(GLOB _page_definitions CONFIGURE_DEPENDS "${SETTINGS_PAGES_DIR}/*.json")
file(GLOB _settings_metadata CONFIGURE_DEPENDS "${SETTINGS_METADATA_DIR}/*.SettingsGroup.json")

# C++ inputs scanned for accessors and C++-defined facts. They are build-only dependencies: editing a Settings source
# regenerates the pages without re-running CMake configure.
file(GLOB _settings_cpp_sources CONFIGURE_DEPENDS "${SETTINGS_METADATA_DIR}/*.cc")
list(APPEND _settings_cpp_sources "${SETTINGS_METADATA_DIR}/SettingsManager.h")

# Custom builds may overlay the page list (SettingsPages.json), add/shadow page
# definitions, and add custom SettingsGroup.json fact metadata.
set(_custom_codegen_args "")
set(CUSTOM_SETTINGS_PAGES_DIR "${CMAKE_SOURCE_DIR}/${QGC_CUSTOM_DIR}/src/AppSettings/pages")
set(CUSTOM_SETTINGS_METADATA_DIR "${CMAKE_SOURCE_DIR}/${QGC_CUSTOM_DIR}/src/Settings")
if(QGC_CUSTOM_BUILD AND IS_DIRECTORY "${CUSTOM_SETTINGS_PAGES_DIR}")
    list(APPEND _custom_codegen_args --custom-pages-dir "${CUSTOM_SETTINGS_PAGES_DIR}")
    file(GLOB _custom_page_definitions CONFIGURE_DEPENDS "${CUSTOM_SETTINGS_PAGES_DIR}/*.json")
    list(APPEND _page_definitions ${_custom_page_definitions})
endif()
if(QGC_CUSTOM_BUILD AND IS_DIRECTORY "${CUSTOM_SETTINGS_METADATA_DIR}")
    list(APPEND _custom_codegen_args --custom-settings-dir "${CUSTOM_SETTINGS_METADATA_DIR}")
    file(GLOB _custom_settings_metadata CONFIGURE_DEPENDS "${CUSTOM_SETTINGS_METADATA_DIR}/*.SettingsGroup.json")
    list(APPEND _settings_metadata ${_custom_settings_metadata})
    file(GLOB _custom_settings_cpp_sources CONFIGURE_DEPENDS "${CUSTOM_SETTINGS_METADATA_DIR}/*.cc")
    list(APPEND _settings_cpp_sources ${_custom_settings_cpp_sources})
endif()

set(_custom_extra_args_keyword "")
if(_custom_codegen_args)
    set(_custom_extra_args_keyword EXTRA_ARGS ${_custom_codegen_args})
endif()

# The generator lists its outputs, so pages added to SettingsPages.json or a custom overlay are picked up
qgc_add_qml_codegen(
    GenerateSettingsQml GENERATE_AT_CONFIGURE LIST_OUTPUTS
    GENERATOR_MODULE tools.generators.settings_qml.generate_pages
    OUTPUT_DIR "${SETTINGS_QML_GEN_DIR}" ${_custom_extra_args_keyword}
    DEPENDS ${_page_definitions} ${_settings_metadata}
    BUILD_DEPENDS ${_settings_cpp_sources}
    COMMENT "Generating QML settings pages from JSON definitions"
)
set(_generated_qml ${GenerateSettingsQml_OUTPUTS})

# ---------------------------------------------------------------------------
# QML module
# ---------------------------------------------------------------------------

qt_add_library(AppSettingsModule STATIC)

# Ensure generated files exist before the module is built
add_dependencies(AppSettingsModule GenerateSettingsQml)

qt_add_qml_module(
    AppSettingsModule
    URI QGroundControl.AppSettings
    VERSION 1.0
    RESOURCE_PREFIX /qml
    DEPENDENCIES
        QGroundControl.Controls
        QGroundControl.LogManager
    QML_FILES
        # Generated from SettingsUI.json definitions (output in build tree)
        ${_generated_qml}
        # Hand-coded pages and components
        AppLogging.qml
        BluetoothSettings.qml
        DebugWindow.qml
        HelpSettings.qml
        LinkConfigurationManager.qml
        LogReplaySettings.qml
        MapProviderSettings.qml
        MavlinkActionSettings.qml
        MavlinkLinkStatus.qml
        SigningKeyManager.qml
        MockLink.qml
        MockLinkSettings.qml
        OfflineMapEditor.qml
        OfflineMapInfo.qml
        OfflineMapSettings.qml
        PX4LogControl.qml
        PX4LogFileManager.qml
        PX4LogUploadSettings.qml
        QmlTest.qml
        SerialSettings.qml
        SettingsPage.qml
        TcpSettings.qml
        UdpSettings.qml
    NO_PLUGIN
)

qt_add_resources(
    ${CMAKE_PROJECT_NAME} appsettings_resources
    PREFIX "/res"
    BASE "${CMAKE_SOURCE_DIR}/resources"
    FILES
        "${CMAKE_SOURCE_DIR}/resources/pencil.svg"
)
