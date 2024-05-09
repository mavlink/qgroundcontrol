find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(AnalyzeViewTest
	STATIC
		LogDownloadTest.cc
		LogDownloadTest.h
		MavlinkLogTest.cc
		MavlinkLogTest.h
)

target_link_libraries(AnalyzeViewTest
	PRIVATE
		Qt6::Core
		AnalyzeView
		MAVLink
		QGC
		Utilities
		Vehicle
	PUBLIC
		qgcunittest
)

target_include_directories(AnalyzeViewTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
