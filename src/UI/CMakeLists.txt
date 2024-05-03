add_subdirectory(preferences)
add_subdirectory(toolbar)

find_package(Qt6 REQUIRED COMPONENTS Core Qml)

add_custom_target(UIQml
	SOURCES
		AppSettings.qml
		ExitWithErrorWindow.qml
		MainRootWindow.qml
		SettingsPagesModel.qml
)
