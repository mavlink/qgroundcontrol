# ---------------------------------------------------------------------------
# Code-generation: produce QML settings pages from JSON definitions
# ---------------------------------------------------------------------------

set(SETTINGS_QML_GENERATOR_DIR "${CMAKE_SOURCE_DIR}/tools/generators/settings_qml")
set(SETTINGS_PAGES_DIR         "${CMAKE_CURRENT_SOURCE_DIR}/pages")
set(SETTINGS_METADATA_DIR      "${CMAKE_SOURCE_DIR}/src/Settings")
set(SETTINGS_QML_GEN_DIR       "${CMAKE_CURRENT_BINARY_DIR}/generated")

# Generator source files (changes here should trigger regeneration)
file(GLOB _generator_sources CONFIGURE_DEPENDS
    "${SETTINGS_QML_GENERATOR_DIR}/*.py"
    "${SETTINGS_QML_GENERATOR_DIR}/templates/*.j2"
    "${CMAKE_SOURCE_DIR}/tools/generators/common/*.py"
    "${CMAKE_SOURCE_DIR}/tools/generators/*.py"
)

# JSON inputs: page definitions + settings metadata
file(GLOB _page_definitions  CONFIGURE_DEPENDS "${SETTINGS_PAGES_DIR}/*.json")
file(GLOB _settings_metadata CONFIGURE_DEPENDS "${SETTINGS_METADATA_DIR}/*.SettingsGroup.json")

# Custom builds may overlay the page list (SettingsPages.json), add/shadow page
# definitions, and add custom SettingsGroup.json fact metadata.
set(_custom_codegen_args "")
set(CUSTOM_SETTINGS_PAGES_DIR    "${CMAKE_SOURCE_DIR}/${QGC_CUSTOM_DIR}/src/AppSettings/pages")
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
endif()

if(_custom_codegen_args)
    # The overlay can add/remove/replace pages, so ask the generator for the
    # resulting output file list instead of hardcoding it.
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -m tools.generators.settings_qml.generate_pages
                --list-outputs ${_custom_codegen_args}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE _generated_qml_names
        RESULT_VARIABLE _list_outputs_result
        ERROR_VARIABLE _list_outputs_error
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT _list_outputs_result EQUAL 0)
        message(FATAL_ERROR "Settings QML generator --list-outputs failed:\n${_list_outputs_error}")
    endif()
    # Python emits CRLF on Windows; execute_process does not normalize it
    string(REPLACE "\r" "" _generated_qml_names "${_generated_qml_names}")
    string(REPLACE "\n" ";" _generated_qml_names "${_generated_qml_names}")
else()
    # Generated QML outputs (must match what generate_pages.py produces)
    set(_generated_qml_names
        ADSBServerSettings.qml
        CommLinksSettings.qml
        FlyViewSettings.qml
        GeneralSettings.qml
        LoggingSettings.qml
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
endif()

set(_custom_extra_args_keyword "")
if(_custom_codegen_args)
    set(_custom_extra_args_keyword EXTRA_ARGS ${_custom_codegen_args})
endif()

qgc_add_qml_codegen(GenerateSettingsQml
    GENERATE_AT_CONFIGURE
    GENERATOR_MODULE tools.generators.settings_qml.generate_pages
    OUTPUT_DIR       "${SETTINGS_QML_GEN_DIR}"
    QML_NAMES        ${_generated_qml_names}
    ${_custom_extra_args_keyword}
    DEPENDS          ${_generator_sources} ${_page_definitions} ${_settings_metadata}
    COMMENT          "Generating QML settings pages from JSON definitions"
)
set(_generated_qml ${GenerateSettingsQml_OUTPUTS})

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
        GcsPositionStatus.qml
        HelpSettings.qml
        LinkConfigurationManager.qml
        LogReplaySettings.qml
        MapProviderSettings.qml
        MavlinkActionSettings.qml
        MavlinkLinkStatus.qml
        SigningKeyManager.qml
        MockLink.qml
        MockLinkSettings.qml
        NmeaGpsSettings.qml
        NtripConnectionSettings.qml
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
