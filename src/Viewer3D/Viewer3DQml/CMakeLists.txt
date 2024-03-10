add_subdirectory(Models3D)
add_subdirectory(Drones)

add_custom_target(Viewer3DQml
	SOURCES
	Viewer3D.qml
	Viewer3DProgressBar.qml
)

