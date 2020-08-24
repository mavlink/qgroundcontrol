
add_library(uas
	UAS.cc
	UAS.h
	UASInterface.h
	UASMessageHandler.cc
	UASMessageHandler.h
)

target_link_libraries(uas
	PRIVATE
		qgc
)

target_include_directories(uas INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

