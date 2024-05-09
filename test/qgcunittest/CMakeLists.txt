find_package(Qt6 REQUIRED COMPONENTS Core Positioning Test Widgets)

qt_add_library(qgcunittest
	STATIC
		# FileDialogTest.cc
		# FileDialogTest.h
		# MainWindowTest.cc
		# MainWindowTest.h
		# MessageBoxTest.cc
		# MessageBoxTest.h
		MultiSignalSpy.cc
		MultiSignalSpy.h
		MultiSignalSpyV2.cc
		MultiSignalSpyV2.h
		UnitTest.cc
		UnitTest.h
)

target_link_libraries(qgcunittest
	PRIVATE
		FactSystem
		MissionManager
		QGC
		Settings
		Vehicle
		VehicleComponents
	PUBLIC
		Qt6::Core
		Qt6::Positioning
		Qt6::Test
		Qt6::Widgets
		MAVLink
		MockLink
)

target_include_directories(qgcunittest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
