
if (ANDROID)
	add_library(qtandroidserialport
		src/qserialport.cpp
		src/qserialport_android.cpp
		src/qserialportinfo.cpp
		src/qserialportinfo_android.cpp
	)

	target_link_libraries(qtandroidserialport
		PRIVATE
			qgc
		PUBLIC
			Qt6::CorePrivate
	)

	target_include_directories(qtandroidserialport PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
endif()
