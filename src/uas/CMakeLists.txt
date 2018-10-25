
add_library(uas
	FileManager.cc
	UAS.cc
	UASMessageHandler.cc

	# HEADERS
	# shouldn't be listed here, but aren't named properly for AUTOMOC
	UAS.h
	UASInterface.h
)

target_link_libraries(uas
	PUBLIC
		qgc
)

target_include_directories(uas PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

