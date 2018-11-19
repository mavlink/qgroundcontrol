
add_library(FlightDisplay
	VideoManager.cc
)

target_link_libraries(FlightDisplay
	PRIVATE
		qgc
)

target_include_directories(FlightDisplay PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
