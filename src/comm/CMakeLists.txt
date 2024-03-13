# NO_SERIAL_LINK # TODO: Make this QGC_NO_SERIAL_LINK

set(EXTRA_SRC)
if(QGC_BUILD_TESTING)
	list(APPEND EXTRA_SRC
		MockLink.cc
		MockLink.h
		MockLinkFTP.cc
		MockLinkFTP.h
		MockLinkMissionItemHandler.cc
		MockLinkMissionItemHandler.h
	)
endif()

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

	${EXTRA_SRC}
)

target_link_libraries(comm
	PUBLIC
		qgc
		Qt6::Location
		Qt6::SerialPort
		Qt6::Test
		Qt6::TextToSpeech
		Qt6::Widgets
		Qt6::Bluetooth
)

option(QGC_ZEROCONF_ENABLED "Enable ZeroConf Compatibility" OFF)
if(QGC_ZEROCONF_ENABLED)
	target_link_libraries(comm PRIVATE qmdnsengine)
	target_include_directories(comm PRIVATE ${CMAKE_SOURCE_DIR}/libs)
endif()

target_compile_definitions(comm PUBLIC QGC_ENABLE_BLUETOOTH)

target_include_directories(comm
    PUBLIC
    	${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/libs/mavlink/include/mavlink/v2.0
        ${CMAKE_SOURCE_DIR}/libs/mavlink/include/mavlink/v2.0/all
        ${CMAKE_SOURCE_DIR}/libs/mavlink/include/mavlink/v2.0/common
)
