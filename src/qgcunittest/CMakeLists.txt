
add_library(qgcunittest
	#FileDialogTest.cc
	#FileManagerTest.cc
	#FlightGearTest.cc
	GeoTest.cc
	LinkManagerTest.cc
	#MainWindowTest.cc
	MavlinkLogTest.cc
	#MessageBoxTest.cc
	MultiSignalSpy.cc
	#RadioConfigTest.cc
	TCPLinkTest.cc
	TCPLoopBackServer.cc
	UnitTest.cc
	UnitTestList.cc
)

target_link_libraries(qgcunittest
	PRIVATE
		qgc
)

target_include_directories(qgcunittest
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
	)

