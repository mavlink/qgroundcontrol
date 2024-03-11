find_package(Qt6 REQUIRED COMPONENTS Core Positioning)

qt_add_library(Geo STATIC
	QGCGeo.cc
	QGCGeo.h
)

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
set(BUILD_BOTH_LIBS OFF CACHE INTERNAL "")
set(BUILD_DOCUMENTATION OFF CACHE INTERNAL "")
set(BUILD_MANPAGES OFF CACHE INTERNAL "")

include(FetchContent)
FetchContent_Declare(geographiclib
	GIT_REPOSITORY https://github.com/geographiclib/geographiclib.git
	GIT_TAG r2.3
	GIT_SHALLOW TRUE
)
FetchContent_GetProperties(geographiclib)
if(NOT geographiclib_POPULATED)
	FetchContent_Populate(geographiclib)
	add_subdirectory(${geographiclib_SOURCE_DIR} ${geographiclib_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
# FetchContent_MakeAvailable(geographiclib)

target_link_libraries(Geo
	PRIVATE
		GeographicLib::GeographicLib
		Utilities
	PUBLIC
		Qt6::Core
		Qt6::Positioning
)

target_include_directories(Geo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
