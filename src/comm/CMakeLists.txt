find_package(Qt6 REQUIRED COMPONENTS Core Network Qml Test Widgets)

qt_add_library(comm STATIC
	LinkConfiguration.cc
	LinkConfiguration.h
	LinkInterface.cc
	LinkInterface.h
	LinkManager.cc
	LinkManager.h
	LogReplayLink.cc
	LogReplayLink.h
	MAVLinkProtocol.cc
	MAVLinkProtocol.h
	QGCMAVLink.cc
	QGCMAVLink.h
	TCPLink.cc
	TCPLink.h
	UdpIODevice.cc
	UdpIODevice.h
	UDPLink.cc
	UDPLink.h
)

if(QGC_BUILD_TESTING)
	target_sources(comm
		PRIVATE
			MockLink.cc
			MockLink.h
			MockLinkFTP.cc
			MockLinkFTP.h
			MockLinkMissionItemHandler.cc
			MockLinkMissionItemHandler.h
	)
endif()

target_link_libraries(comm
	PRIVATE
		Qt6::Qml
		Qt6::Test
		Qt6::Widgets
		AirLink
		Settings
		Vehicle
	PUBLIC
		Qt6::Core
		Qt6::Network
		qgc
		QmlControls
		Utilities
)

if(NOT MOBILE)
	target_link_libraries(comm
		PRIVATE
			gps
			PositionManager
	)
endif()

option(QGC_NO_SERIAL_LINK "Disable Serial Links" OFF)
if(QGC_NO_SERIAL_LINK)
	target_compile_definitions(comm PUBLIC NO_SERIAL_LINK)
else()
	if(ANDROID)
		add_subdirectory(${CMAKE_SOURCE_DIR}/libs/qtandroidserialport qtandroidserialport.build)
	    target_link_libraries(comm PUBLIC qtandroidserialport)
	else()
		find_package(Qt6 REQUIRED COMPONENTS SerialPort)
		target_link_libraries(comm PUBLIC Qt6::SerialPort)
	endif()
	target_sources(comm
		PRIVATE
			QGCSerialPortInfo.cc
			QGCSerialPortInfo.h
			SerialLink.cc
			SerialLink.h
	)
endif()

# option(QGC_NO_SERIAL_LINK "Enable Bluetooth Links" ON)
target_compile_definitions(comm PUBLIC QGC_ENABLE_BLUETOOTH)
find_package(Qt6 REQUIRED COMPONENTS Bluetooth)
target_link_libraries(comm PUBLIC Qt6::Bluetooth)
target_sources(comm
	PRIVATE
		BluetoothLink.cc
		BluetoothLink.h
)

option(QGC_ZEROCONF_ENABLED "Enable ZeroConf Compatibility" OFF)
if(QGC_ZEROCONF_ENABLED)
	set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
	set(BUILD_DOC OFF CACHE INTERNAL "")
	set(BUILD_EXAMPLES OFF CACHE INTERNAL "")
	set(BUILD_TESTS OFF CACHE INTERNAL "")

	include(FetchContent)
	FetchContent_Declare(qmdnsengine
		GIT_REPOSITORY https://github.com/nitroshare/qmdnsengine.git
		GIT_TAG d61e497
		GIT_SHALLOW TRUE
	)
	FetchContent_MakeAvailable(qmdnsengine)

	target_link_libraries(comm PRIVATE qmdnsengine)
	target_compile_definitions(comm PUBLIC QGC_ZEROCONF_ENABLED)
endif()

# if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
# 	target_compile_options(comm PRIVATE -Wno-address-of-packed-member)
# endif()

target_include_directories(comm
    PUBLIC
    	${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/libs/mavlink/include/mavlink/v2.0
        ${CMAKE_SOURCE_DIR}/libs/mavlink/include/mavlink/v2.0/all
        ${CMAKE_SOURCE_DIR}/libs/mavlink/include/mavlink/v2.0/common
)
