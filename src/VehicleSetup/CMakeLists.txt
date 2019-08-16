
add_library(VehicleSetup
	Bootloader.cc
	FirmwareImage.cc
	FirmwareUpgradeController.cc
	JoystickConfigController.cc
	PX4FirmwareUpgradeThread.cc
	VehicleComponent.cc
)

add_custom_target(VehicleSetupQml
SOURCES
	JoystickConfigCalibration.qml
	JoystickConfigGeneral.qml
	SetupParameterEditor.qml
	VehicleSummary.qml
	JoystickConfigButtons.qml
	PX4FlowSensor.qml
	FirmwareUpgrade.qml
	JoystickConfig.qml
	JoystickConfigAdvanced.qml
	SetupView.qml
)

target_link_libraries(VehicleSetup
	PUBLIC
		qgc
)

target_include_directories(VehicleSetup INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
