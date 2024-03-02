qt_add_library(zlib STATIC)

if(WIN32)
	target_link_libraries(zlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/windows/lib/zlibstatic.lib)
	target_include_directories(zlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/windows/include)
else()
	find_package(ZLIB)
	target_link_libraries(zlib PUBLIC ZLIB::ZLIB)
endif()
