
add_library(mcu
	MCUManager.cc
)

target_link_libraries(mcu
		qgc
)

target_include_directories(mcu INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})




