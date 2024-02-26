

qt_add_library(gps STATIC
	Drivers/src/ashtech.cpp
	Drivers/src/gps_helper.cpp
	Drivers/src/mtk.cpp
	Drivers/src/rtcm.cpp
	Drivers/src/sbf.cpp
	Drivers/src/ubx.cpp
	GPSManager.cc
	GPSProvider.cc
	RTCM/RTCMMavlink.cc
)

target_link_libraries(gps
	Qt6::Core
	Qt6::Location
	Qt6::SerialPort
	Qt6::Svg
	Qt6::TextToSpeech

	qgc
)

target_include_directories(gps INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

