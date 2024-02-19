find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(qtandroidserialport
	STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/qserialport.cpp ${CMAKE_CURRENT_SOURCE_DIR}/qserialport.h ${CMAKE_CURRENT_SOURCE_DIR}/qserialport_p.h
	${CMAKE_CURRENT_SOURCE_DIR}/qserialport_android.cpp ${CMAKE_CURRENT_SOURCE_DIR}/qserialport_android_p.h
	${CMAKE_CURRENT_SOURCE_DIR}/qserialportinfo.cpp ${CMAKE_CURRENT_SOURCE_DIR}/qserialportinfo.h ${CMAKE_CURRENT_SOURCE_DIR}/qserialportinfo_p.h
	${CMAKE_CURRENT_SOURCE_DIR}/qserialportinfo_android.cpp
)

target_link_libraries(qtandroidserialport
	PUBLIC
		Qt6::Core
		qgc
)

target_include_directories(qtandroidserialport
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR})
