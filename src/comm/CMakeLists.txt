
set(EXTRA_SRC)
if(BUILD_TESTING)
	list(APPEND EXTRA_SRC
		MockLink.cc
		MockLinkFileServer.cc
		MockLinkMissionItemHandler.cc
	)
endif()

add_library(comm
	#BluetoothLink.cc
	LinkConfiguration.cc
	LinkInterface.cc
	LinkManager.cc
	LogReplayLink.cc
	MavlinkMessagesTimer.cc
	MAVLinkProtocol.cc
	QGCJSBSimLink.cc
	QGCMAVLink.cc
	QGCSerialPortInfo.cc
	QGCXPlaneLink.cc
	SerialLink.cc
	TCPLink.cc
	UDPLink.cc
	UdpIODevice.cc

	${EXTRA_SRC}

	# HEADERS
	# shouldn't be listed here, but aren't named properly for AUTOMOC
	QGCHilLink.h
	QGCJSBSimLink.h
)

target_link_libraries(comm
	PRIVATE
		qgc
	PUBLIC
		Qt5::Location
		Qt5::SerialPort
		Qt5::Test
		Qt5::TextToSpeech
		Qt5::Widgets
)

target_include_directories(comm INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

