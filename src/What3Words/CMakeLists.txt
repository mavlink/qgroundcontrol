add_library(What3Words
	What3Words.cc
)

target_link_libraries(What3Words
	PUBLIC
		qgc

		Qt5::Network
)

target_include_directories(What3Words INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
