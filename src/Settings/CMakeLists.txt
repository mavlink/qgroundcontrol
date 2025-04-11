set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/ADSBVehicleManagerSettings.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ADSBVehicleManagerSettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AppSettings.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/AppSettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AutoConnectSettings.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/AutoConnectSettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/BatteryIndicatorSettings.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/BatteryIndicatorSettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/BrandImageSettings.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/BrandImageSettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FirmwareUpgradeSettings.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/FirmwareUpgradeSettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FlightMapSettings.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/FlightMapSettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FlightModeSettings.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/FlightModeSettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FlyViewSettings.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/FlyViewSettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GimbalControllerSettings.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/GimbalControllerSettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MapsSettings.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MapsSettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MavlinkActionsSettings.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MavlinkActionsSettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MavlinkSettings.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MavlinkSettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/OfflineMapsSettings.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/OfflineMapsSettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PlanViewSettings.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/PlanViewSettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/RemoteIDSettings.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/RemoteIDSettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/RTKSettings.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/RTKSettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SettingsGroup.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/SettingsGroup.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SettingsManager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/SettingsManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UnitsSettings.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/UnitsSettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoSettings.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/VideoSettings.h
    CACHE INTERNAL "" FORCE
)

if(NOT QGC_DISABLE_APM_MAVLINK)
    set(QGC_SOURCES
        ${QGC_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/APMMavlinkStreamRateSettings.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/APMMavlinkStreamRateSettings.h
        CACHE INTERNAL "" FORCE
    )
endif()

if(QGC_VIEWER3D)
    set(QGC_SOURCES
        ${QGC_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/Viewer3DSettings.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/Viewer3DSettings.h
        CACHE INTERNAL "" FORCE
    )
endif()

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)
