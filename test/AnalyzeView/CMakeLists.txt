find_package(Qt6 REQUIRED COMPONENTS Core Test)

qt_add_library(AnalyzeViewTest
	STATIC
		ExifParserTest.cc
		ExifParserTest.h
		LogDownloadTest.cc
		LogDownloadTest.h
		MavlinkLogTest.cc
		MavlinkLogTest.h
)

target_link_libraries(AnalyzeViewTest
	PRIVATE
		Qt6::Core
		Qt6::Test
		AnalyzeView
		MAVLink
		QGC
		Utilities
		Vehicle
	PUBLIC
		qgcunittest
)

target_include_directories(AnalyzeViewTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_resources(AnalyzeViewTest "AnalyzeViewTest"
	PREFIX "/"
	FILES
		DSCN0010.jpg
)
