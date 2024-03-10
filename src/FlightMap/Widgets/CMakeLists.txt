find_package(Qt6 REQUIRED COMPONENTS Core Qml)

add_custom_target(FlightMapWidgetsQml
	SOURCES
		CenterMapDropButton.qml
		CenterMapDropPanel.qml
		HorizontalCompassAttitude.qml
		MapFitFunctions.qml
		PhotoVideoControl.qml
		QGCArtificialHorizon.qml
		QGCAttitudeHUD.qml
		QGCAttitudeWidget.qml
		QGCCompassWidget.qml
		QGCPitchIndicator.qml
		QGCWaypointEditor.qml
		VerticalCompassAttitude.qml
        CompassDial.qml
        CompassHeadingIndicator.qml
        IntegratedAttitudeIndicator.qml
        IntegratedCompassAttitude.qml
)

