find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(qgcandroid
	STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/AndroidInit.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/AndroidInterface.cc ${CMAKE_CURRENT_SOURCE_DIR}/AndroidInterface.h
	${CMAKE_CURRENT_SOURCE_DIR}/AndroidSerial.cpp ${CMAKE_CURRENT_SOURCE_DIR}/AndroidSerial.h
)

target_link_libraries(qgcandroid
	PRIVATE
		Qt6::SerialPortPrivate
		Joystick
	PUBLIC
		Qt6::Core
		Qt6::SerialPort
)

target_include_directories(qgcandroid
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
)
