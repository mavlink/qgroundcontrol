# ============================================================================
# Settings Module
# Application, vehicle, and user preference settings management
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ADSBVehicleManagerSettings.cc
        ADSBVehicleManagerSettings.h
        AppSettings.cc
        AppSettings.h
        AutoConnectSettings.cc
        AutoConnectSettings.h
        BatteryIndicatorSettings.cc
        BatteryIndicatorSettings.h
        BrandImageSettings.cc
        BrandImageSettings.h
        MavlinkActionsSettings.cc
        MavlinkActionsSettings.h
        FirmwareUpgradeSettings.cc
        FirmwareUpgradeSettings.h
        FlightMapSettings.cc
        FlightMapSettings.h
        FlightModeSettings.cc
        FlightModeSettings.h
        FlyViewSettings.cc
        FlyViewSettings.h
        GimbalControllerSettings.cc
        GimbalControllerSettings.h
        MavlinkSettings.cc
        MavlinkSettings.h
        OfflineMapsSettings.cc
        OfflineMapsSettings.h
        PlanViewSettings.cc
        PlanViewSettings.h
        RemoteIDSettings.cc
        RemoteIDSettings.h
        RTKSettings.cc
        RTKSettings.h
        SettingsGroup.cc
        SettingsGroup.h
        SettingsManager.cc
        SettingsManager.h
        UnitsSettings.cc
        UnitsSettings.h
        VideoSettings.cc
        VideoSettings.h
        MapsSettings.cc
        MapsSettings.h
        Viewer3DSettings.cc
        Viewer3DSettings.h
)

# ----------------------------------------------------------------------------
# APM-Specific Settings
# ----------------------------------------------------------------------------
if(NOT QGC_DISABLE_APM_MAVLINK)
    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            APMMavlinkStreamRateSettings.cc
            APMMavlinkStreamRateSettings.h
    )
endif()

# ----------------------------------------------------------------------------
# Settings Definition Resources
# ----------------------------------------------------------------------------
file(GLOB_RECURSE JSON_SETTINGS_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.json)
qt_add_resources(${CMAKE_PROJECT_NAME} json_app_settings
    PREFIX "/json"
    FILES ${JSON_SETTINGS_FILES}
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
