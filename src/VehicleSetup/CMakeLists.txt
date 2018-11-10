
add_library(VehicleSetup
	Bootloader.cc
	FirmwareImage.cc
	FirmwareUpgradeController.cc
	JoystickConfigController.cc
	PX4FirmwareUpgradeThread.cc
	VehicleComponent.cc
)

target_link_libraries(VehicleSetup
	PUBLIC
		qgc
)

target_include_directories(VehicleSetup INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
