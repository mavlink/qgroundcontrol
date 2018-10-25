
if(QGC_AIRMAP)
	add_library(AirspaceManagement
		AirspaceManagement/AirspaceAdvisoryProvider.cc
		AirspaceManagement/AirspaceFlightPlanProvider.cc
		AirspaceManagement/AirspaceManager.cc
		AirspaceManagement/AirspaceRestriction.cc
		AirspaceManagement/AirspaceRestrictionProvider.cc
		AirspaceManagement/AirspaceRulesetsProvider.cc
		AirspaceManagement/AirspaceVehicleManager.cc
		AirspaceManagement/AirspaceWeatherInfoProvider.cc
	)
endif()

