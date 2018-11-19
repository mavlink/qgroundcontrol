
add_library(ui

	HILDockWidget.cc
	linechart/ChartPlot.cc
	linechart/IncrementalPlot.cc
	linechart/LinechartPlot.cc
	linechart/Linecharts.cc
	linechart/LinechartWidget.cc
	linechart/Scrollbar.cc
	linechart/ScrollZoomer.cc

	MainWindow.cc
	MAVLinkDecoder.cc
	MultiVehicleDockWidget.cc
	QGCHilConfiguration.cc
	QGCHilFlightGearConfiguration.cc
	QGCHilJSBSimConfiguration.cc
	QGCHilXPlaneConfiguration.cc
	QGCMapRCToParamDialog.cpp
	QGCMAVLinkInspector.cc
	QGCMAVLinkLogPlayer.cc
	QGCPluginHost.cc
	QGCUASFileView.cc
	QGCUASFileViewMulti.cc

	uas/QGCUnconnectedInfoWidget.cc

	# HEADERS
	# shouldn't be listed here, but aren't named properly for AUTOMOC
	QGCHilFlightGearConfiguration.h
	QGCHilJSBSimConfiguration.h

	# UI
	MainWindow.ui
	MultiVehicleDockWidget.ui
	QGCHilConfiguration.ui
	QGCHilFlightGearConfiguration.ui
	QGCHilJSBSimConfiguration.ui
	QGCHilXPlaneConfiguration.ui
	QGCMapRCToParamDialog.ui
	QGCMAVLinkInspector.ui
	QGCMAVLinkLogPlayer.ui
	QGCPluginHost.ui
	QGCUASFileView.ui
	QGCUASFileViewMulti.ui
	QMap3D.ui

	uas/QGCUnconnectedInfoWidget.ui
)

target_link_libraries(ui
	PUBLIC
		qgc
	PRIVATE
		qwt
)

target_include_directories(ui
	INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}
                ${CMAKE_CURRENT_BINARY_DIR}
	PRIVATE
		linechart
	)

