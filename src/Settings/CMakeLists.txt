
add_library(Settings
	AppSettings.cc
	AutoConnectSettings.cc
	BrandImageSettings.cc
	FlightMapSettings.cc
	FlyViewSettings.cc
	PlanViewSettings.cc
	RTKSettings.cc
	SettingsGroup.cc
	SettingsManager.cc
	UnitsSettings.cc
	VideoSettings.cc
)

target_link_libraries(Settings
	PUBLIC
		qgc
)

target_include_directories(Settings PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

