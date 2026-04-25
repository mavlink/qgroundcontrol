find_package(Python3 REQUIRED COMPONENTS Interpreter)

# ---------------------------------------------------------------------------
# Code-generation: produce QML settings pages from JSON definitions
# ---------------------------------------------------------------------------

set(SETTINGS_QML_GENERATOR_DIR "${CMAKE_SOURCE_DIR}/tools/generators/settings_qml")
set(SETTINGS_PAGES_DIR         "${CMAKE_CURRENT_SOURCE_DIR}/pages")
set(SETTINGS_METADATA_DIR      "${CMAKE_SOURCE_DIR}/src/Settings")
set(SETTINGS_QML_GEN_DIR       "${CMAKE_CURRENT_BINARY_DIR}/generated")

# Generator source files (changes here should trigger regeneration)
file(GLOB _generator_sources "${SETTINGS_QML_GENERATOR_DIR}/*.py")

# JSON inputs: page definitions + settings metadata
file(GLOB _page_definitions  "${SETTINGS_PAGES_DIR}/*.json")
file(GLOB _settings_metadata "${SETTINGS_METADATA_DIR}/*.SettingsGroup.json")

# Generated QML outputs (must match what generate_pages.py produces)
set(_generated_qml_names
    ADSBServerSettings.qml
    CommLinksSettings.qml
    FlyViewSettings.qml
    GeneralSettings.qml
    MapsSettings.qml
    NTRIPSettings.qml
    PlanViewSettings.qml
    PX4LogTransferSettings.qml
    RemoteIDSettings.qml
    SettingsPagesModel.qml
    TelemetrySettings.qml
    VideoSettings.qml
    Viewer3DSettings.qml
)

# Full paths in the build tree
set(_generated_qml "")
foreach(_name IN LISTS _generated_qml_names)
    list(APPEND _generated_qml "${SETTINGS_QML_GEN_DIR}/${_name}")
endforeach()

add_custom_command(
    OUTPUT  ${_generated_qml}
    COMMAND ${Python3_EXECUTABLE} -m tools.generators.settings_qml.generate_pages
            --output-dir "${SETTINGS_QML_GEN_DIR}"
    DEPENDS ${_generator_sources} ${_page_definitions} ${_settings_metadata}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Generating QML settings pages from JSON definitions"
    VERBATIM
)

add_custom_target(GenerateSettingsQml DEPENDS ${_generated_qml})

# Set QT_RESOURCE_ALIAS on each generated file so qt_add_qml_module places
# them at the correct QRC path despite living in the build tree.
foreach(_name IN LISTS _generated_qml_names)
    set_source_files_properties("${SETTINGS_QML_GEN_DIR}/${_name}"
        PROPERTIES QT_RESOURCE_ALIAS "${_name}"
    )
endforeach()

# ---------------------------------------------------------------------------
# QML module
# ---------------------------------------------------------------------------

qt_add_library(AppSettingsModule STATIC)

# Ensure generated files exist before the module is built
add_dependencies(AppSettingsModule GenerateSettingsQml)

qt_add_qml_module(AppSettingsModule
    URI QGroundControl.AppSettings
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        # Generated from SettingsUI.json definitions (output in build tree)
        ${_generated_qml}

        # Hand-coded pages and components
        BluetoothSettings.qml
        DebugWindow.qml
        GcsPositionStatus.qml
        HelpSettings.qml
        LinkConfigurationManager.qml
        LogReplaySettings.qml
        MapProviderSettings.qml
        MavlinkActionSettings.qml
        MavlinkLinkStatus.qml
        MockLink.qml
        MockLinkSettings.qml
        NmeaGpsSettings.qml
        NtripConnectionStatus.qml
        NtripMountpointBrowser.qml
        NtripServerSettings.qml
        OfflineMapEditor.qml
        OfflineMapInfo.qml
        OfflineMapSettings.qml
        PX4LogControl.qml
        PX4LogFileManager.qml
        PX4LogUploadSettings.qml
        QmlTest.qml
        RemoteIDGpsLocation.qml
        SerialSettings.qml
        SettingsPage.qml
        TcpSettings.qml
        UdpSettings.qml
    NO_PLUGIN
)

qt_add_resources(${CMAKE_PROJECT_NAME} appsettings_resources
    PREFIX "/res"
    BASE "${CMAKE_SOURCE_DIR}/resources"
    FILES
        "${CMAKE_SOURCE_DIR}/resources/pencil.svg"
)
