find_package(Qt6 REQUIRED COMPONENTS Core Positioning Test Widgets)

qt_add_library(qgcunittest
	STATIC
		# FileDialogTest.cc
		# FileDialogTest.h
		ComponentInformationCacheTest.cc
		ComponentInformationCacheTest.h
		ComponentInformationTranslationTest.cc
		ComponentInformationTranslationTest.h
		# MainWindowTest.cc
		# MainWindowTest.h
		MavlinkLogTest.cc
		MavlinkLogTest.h
		# MessageBoxTest.cc
		# MessageBoxTest.h
		MultiSignalSpy.cc
		MultiSignalSpy.h
		MultiSignalSpyV2.cc
		MultiSignalSpyV2.h
		# RadioConfigTest.cc
		# RadioConfigTest.h
		UnitTest.cc
		UnitTest.h
)

target_link_libraries(qgcunittest
	PRIVATE
		Qt6::Test
		FactSystem
		QGC
		Settings
		Vehicle
		VehicleComponents
	PUBLIC
		Qt6::Core
		Qt6::Positioning
		Qt6::Widgets
		AutoPilotPlugins
		Comms
		MissionManager
		MockLink
)

qt_add_resources(qgcunittest
	"qgcunittest_res"
	FILES
		TranslationTest.json
		TranslationTest_de_DE.ts
)

target_include_directories(qgcunittest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
