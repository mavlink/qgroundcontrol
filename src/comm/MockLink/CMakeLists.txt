find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(MockLink STATIC)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	find_package(Qt6 REQUIRED COMPONENTS Positioning)

	target_sources(MockLink
		PRIVATE
			MockLink.cc
			MockLink.h
			MockLinkFTP.cc
			MockLinkFTP.h
			MockLinkMissionItemHandler.cc
			MockLinkMissionItemHandler.h
	)

	target_link_libraries(MockLink
		PRIVATE
			FirmwarePlugin
			qgc
			Utilities
		PUBLIC
			Qt6::Core
			Qt6::Positioning
			comm
	)

	target_include_directories(MockLink PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()
