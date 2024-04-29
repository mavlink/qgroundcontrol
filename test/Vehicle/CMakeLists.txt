find_package(Qt6 REQUIRED COMPONENTS Core Test)

qt_add_library(VehicleTest
	STATIC
		FTPManagerTest.cc
		FTPManagerTest.h
		RequestMessageTest.cc
		RequestMessageTest.h
		SendMavCommandWithHandlerTest.cc
		SendMavCommandWithHandlerTest.h
		SendMavCommandWithSignallingTest.cc
		SendMavCommandWithSignallingTest.h
		VehicleLinkManagerTest.cc
		VehicleLinkManagerTest.h
)

target_link_libraries(VehicleTest
	PRIVATE
		Qt6::Test
		qgc
	PUBLIC
		comm
		qgcunittest
		Vehicle
)

target_include_directories(VehicleTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
