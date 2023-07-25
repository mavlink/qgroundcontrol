
add_library(qgcunittest
	#FileDialogTest.cc
	#FileDialogTest.h
	#FileManagerTest.cc
	#FileManagerTest.h
	ComponentInformationCacheTest.cc
	ComponentInformationCacheTest.h
	GeoTest.cc
	GeoTest.h
	#MainWindowTest.cc
	#MainWindowTest.h
	MavlinkLogTest.cc
	MavlinkLogTest.h
	#MessageBoxTest.cc
	#MessageBoxTest.h
	MultiSignalSpy.cc
	MultiSignalSpy.h
	MultiSignalSpyV2.cc
	MultiSignalSpyV2.h
	#RadioConfigTest.cc
	#RadioConfigTest.h
	UnitTest.cc
	UnitTest.h
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

