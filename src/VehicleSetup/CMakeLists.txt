
add_library(VehicleSetup
	Bootloader.cc
	Bootloader.h
	FirmwareImage.cc
	FirmwareImage.h
	FirmwareUpgradeController.cc
	FirmwareUpgradeController.h
	JoystickConfigController.cc
	JoystickConfigController.h
	PX4FirmwareUpgradeThread.cc
	PX4FirmwareUpgradeThread.h
	VehicleComponent.cc
	VehicleComponent.h
)

add_custom_target(VehicleSetupQml
	SOURCES
		FirmwareUpgrade.qml
		JoystickConfigAdvanced.qml
		JoystickConfigButtons.qml
		JoystickConfigCalibration.qml
		JoystickConfigGeneral.qml
		JoystickConfig.qml
		PX4FlowSensor.qml
		SetupParameterEditor.qml
		SetupView.qml
		VehicleSummary.qml
)

target_link_libraries(VehicleSetup
	PRIVATE
		compression
	PUBLIC
		qgc
)

target_include_directories(VehicleSetup INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
