
add_library(api
	QGCCorePlugin.cc
	QGCOptions.cc
	QGCSettings.cc
	QmlComponentInfo.cc
)

target_link_libraries(api
	PUBLIC
		qgc
)

target_include_directories(api PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

