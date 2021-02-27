
add_library(NTRIP
	NTRIP.cc
	NTRIP.h
)

target_link_libraries(NTRIP
	PUBLIC
		qgc
)

target_include_directories(NTRIP
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
	)


