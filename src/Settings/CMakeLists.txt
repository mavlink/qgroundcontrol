if (NOT QGC_DISABLE_APM_MAVLINK)
	set(APM_MAVLINK_SR_SETTINGS_SRC
		APMMavlinkStreamRateSettings.cc
		APMMavlinkStreamRateSettings.h

			)
endif()

add_library(Settings
	ADSBVehicleManagerSettings.cc
	ADSBVehicleManagerSettings.h
	${APM_MAVLINK_SR_SETTINGS_SRC}
	AppSettings.cc
	AppSettings.h
	AutoConnectSettings.cc
	AutoConnectSettings.h
	BrandImageSettings.cc
	BrandImageSettings.h
	FirmwareUpgradeSettings.cc
	FirmwareUpgradeSettings.h
	FlightMapSettings.cc
	FlightMapSettings.h
	FlyViewSettings.cc
	FlyViewSettings.h
	OfflineMapsSettings.cc
	OfflineMapsSettings.h
	PlanViewSettings.cc
	PlanViewSettings.h
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
	RemoteIDSettings.h
	RemoteIDSettings.cc
)

target_link_libraries(Settings
	PUBLIC
		qgc
)
if (NOT QGC_DISABLE_UVC)
	target_link_libraries(Settings
			PUBLIC
			Qt5::Multimedia
			)
endif ()
target_include_directories(Settings PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

