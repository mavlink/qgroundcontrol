if(NOT ANDROID)
	return()
endif()

find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(qtandroidserialport STATIC
	qserialport.cpp
	qserialport.h
	qserialport_android.cpp
	qserialport_p.h
	qserialportglobal.h
	qserialportinfo.cpp
	qserialportinfo.h
	qserialportinfo_p.h
	qserialportinfo_android.cpp
	qtserialportexports.h
	qtserialportversion.h
)

target_link_libraries(qtandroidserialport
	PRIVATE
		QGC
	PUBLIC
		Qt6::Core
		Qt6::CorePrivate
)

target_include_directories(qtandroidserialport PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# target_compile_definitions(qtandroidserialport PUBLIC QIODEVICE_DEBUG)
