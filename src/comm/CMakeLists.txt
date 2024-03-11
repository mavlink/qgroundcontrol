find_package(Qt6 REQUIRED COMPONENTS Bluetooth Core Network Test)

# NO_SERIAL_LINK # TODO: Make this QGC_NO_SERIAL_LINK

qt_add_library(comm STATIC
	BluetoothLink.cc
	BluetoothLink.h
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
	QGCSerialPortInfo.cc
	QGCSerialPortInfo.h
	SerialLink.cc
	SerialLink.h
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
		Qt6::Test
	PUBLIC
		qgc
		Qt6::Widgets
		Qt6::Bluetooth
		Qt6::Network
)

if(ANDROID)
    target_link_libraries(comm PUBLIC qtandroidserialport)
else()
	target_link_libraries(comm PUBLIC Qt6::SerialPort)
endif()

option(QGC_ZEROCONF_ENABLED "Enable ZeroConf Compatibility" OFF)
if(QGC_ZEROCONF_ENABLED)
	set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
	set(BUILD_DOC OFF CACHE INTERNAL "")
	set(BUILD_EXAMPLES OFF CACHE INTERNAL "")
	set(BUILD_TESTS OFF CACHE INTERNAL "")

	include(FetchContent)
	FetchContent_Declare(
        qmdnsengine
		GIT_REPOSITORY https://github.com/nitroshare/qmdnsengine.git
		GIT_TAG d61e497
	)
	FetchContent_MakeAvailable(qmdnsengine)

	target_link_libraries(comm PRIVATE qmdnsengine)
endif()

target_compile_definitions(comm PUBLIC QGC_ENABLE_BLUETOOTH)

target_include_directories(comm
    PUBLIC
    	${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/libs/mavlink/include/mavlink/v2.0
        ${CMAKE_SOURCE_DIR}/libs/mavlink/include/mavlink/v2.0/all
        ${CMAKE_SOURCE_DIR}/libs/mavlink/include/mavlink/v2.0/common
)
