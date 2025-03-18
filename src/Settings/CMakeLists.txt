find_package(Qt6 REQUIRED COMPONENTS Core Multimedia Qml QmlIntegration)

target_sources(QGC
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

if(NOT QGC_DISABLE_APM_MAVLINK)
    target_sources(QGC
        PRIVATE
            APMMavlinkStreamRateSettings.cc
            APMMavlinkStreamRateSettings.h
    )
endif()

target_link_libraries(QGC
    PRIVATE
        Qt6::Multimedia
    PUBLIC
        Qt6::Core
        Qt6::Qml
        Qt6::QmlIntegration
)

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
