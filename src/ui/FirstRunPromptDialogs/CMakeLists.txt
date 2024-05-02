find_package(Qt6 REQUIRED COMPONENTS Core Qml)

add_custom_target(FirstRunPromptDialogs
	SOURCES
		FirstRunPrompt.qml
		OfflineVehicleFirstRunPrompt.qml
		UnitsFirstRunPrompt.qml
)
