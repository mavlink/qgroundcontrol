add_subdirectory(preferences)
add_subdirectory(toolbar)

add_custom_target(uiQml
	SOURCES
		AppSettings.qml
		ExitWithErrorWindow.qml
		MainRootWindow.qml
)
