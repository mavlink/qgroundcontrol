find_package(Qt6 REQUIRED COMPONENTS Core)

include(FetchContent)
FetchContent_Declare(libevents
	GIT_REPOSITORY https://github.com/mavlink/libevents.git
	GIT_TAG main
	GIT_SHALLOW TRUE
	SOURCE_SUBDIR libs/cpp
)
FetchContent_MakeAvailable(libevents)

qt_add_library(LibEventsWrapper STATIC
	EventHandler.cc
	EventHandler.h
	HealthAndArmingCheckReport.cc
	HealthAndArmingCheckReport.h
	logging.cpp
)

target_link_libraries(LibEventsWrapper
	PRIVATE
		QmlControls
		Utilities
	PUBLIC
		Qt6::Core
		libevents
		MAVLink
)

target_include_directories(LibEventsWrapper
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		${libevents_SOURCE_DIR}/libs/cpp
)
