add_subdirectory(MockLink)

find_package(Qt6 REQUIRED COMPONENTS Core Network Qml Test Widgets)

qt_add_library(Comms STATIC
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

target_link_libraries(Comms
	PRIVATE
		Qt6::Qml
		Qt6::Test
		Qt6::Widgets
		AirLink
		MockLink
		Settings
		Vehicle
	PUBLIC
		Qt6::Core
		Qt6::Network
		QGC
		QmlControls
		Utilities
)

target_include_directories(Comms PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

############# Serial

if(QGC_NO_SERIAL_LINK)
	target_compile_definitions(Comms PUBLIC NO_SERIAL_LINK)
else()
	target_sources(Comms
		PRIVATE
			QGCSerialPortInfo.cc
			QGCSerialPortInfo.h
			SerialLink.cc
			SerialLink.h
	)
	target_link_libraries(Comms
		PRIVATE
			GPS
			PositionManager
	)
	if(ANDROID)
		add_subdirectory(${CMAKE_SOURCE_DIR}/libs/qtandroidserialport qtandroidserialport.build)
	    target_link_libraries(Comms PUBLIC qtandroidserialport)
	else()
		find_package(Qt6 REQUIRED COMPONENTS SerialPort)
		target_link_libraries(Comms PUBLIC Qt6::SerialPort)
	endif()
endif()

############# Bluetooth

option(QGC_ENABLE_BLUETOOTH "Enable Bluetooth Links" ON)
if(QGC_ENABLE_BLUETOOTH)
	find_package(Qt6 REQUIRED COMPONENTS Bluetooth)
	target_link_libraries(Comms PUBLIC Qt6::Bluetooth)
	target_sources(Comms
		PRIVATE
			BluetoothLink.cc
			BluetoothLink.h
	)
	target_compile_definitions(Comms PUBLIC QGC_ENABLE_BLUETOOTH)
endif()

############# ZEROConf

option(QGC_ZEROCONF_ENABLED "Enable ZeroConf Compatibility" OFF)
if(QGC_ZEROCONF_ENABLED)
	set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
	set(BUILD_DOC OFF CACHE INTERNAL "")
	set(BUILD_EXAMPLES OFF CACHE INTERNAL "")
	set(BUILD_TESTS OFF CACHE INTERNAL "")

	include(FetchContent)
	FetchContent_Declare(qmdnsengine
		GIT_REPOSITORY https://github.com/nitroshare/qmdnsengine.git
		GIT_TAG 4e54bc86c8ed2d4fa2e7449d4ba6a6a2742d9eb1
	)
	FetchContent_MakeAvailable(qmdnsengine)

	target_link_libraries(Comms PRIVATE qmdnsengine)
	target_compile_definitions(Comms PUBLIC QGC_ZEROCONF_ENABLED)
endif()

############# MAVLink

include(FetchContent)
FetchContent_Declare(MAVLink
	GIT_REPOSITORY https://github.com/mavlink/c_library_v2.git
	GIT_TAG 878a0660666bbb7b982bad4f0d10fad88b1b82d1
)
FetchContent_MakeAvailable(MAVLink)

target_include_directories(Comms
    PUBLIC
        ${mavlink_SOURCE_DIR}
        ${mavlink_SOURCE_DIR}/all
        ${mavlink_SOURCE_DIR}/common
)

# if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
# 	target_compile_options(comm PRIVATE -Wno-address-of-packed-member)
# endif()
