
add_library(FlightDisplay
	VideoManager.cc
)

target_link_libraries(FlightDisplay
	PRIVATE
		qgc
)

add_custom_target(FligthDisplayQml
SOURCES
	FlightDisplayView.qml       
	FlightDisplayViewDummy.qml  
	FlightDisplayViewMap.qml    
	FlightDisplayViewUVC.qml      
	FlightDisplayViewVideo.qml    
	FlightDisplayViewWidgets.qml  
	GuidedActionConfirm.qml       
	GuidedActionList.qml         
	GuidedActionsController.qml  
	GuidedAltitudeSlider.qml     
	MultiVehicleList.qml         
	PreFlightBatteryCheck.qml        
	PreFlightGPSCheck.qml
	PreFlightRCCheck.qml             
	PreFlightSensorsHealthCheck.qml  
	PreFlightSoundCheck.qml
	VirtualJoystick.qml
)

target_include_directories(FlightDisplay PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
