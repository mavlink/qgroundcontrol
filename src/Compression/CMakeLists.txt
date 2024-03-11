qt_add_library(compression STATIC
	QGCLZMA.cc
	QGCLZMA.h
	QGCZlib.cc
	QGCZlib.h
)

set(ZLIB_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")

if(QGC_FETCH_DEPENDS)
	include(FetchContent)
	FetchContent_Declare(zlib
		GIT_REPOSITORY https://github.com/madler/zlib.git
		GIT_TAG v1.3.1
		GIT_SHALLOW TRUE
	)
	FetchContent_MakeAvailable(zlib)
else()
	qt_add_library(zlib STATIC)
	if(WIN32)
		target_link_libraries(zlib PUBLIC ${CMAKE_SOURCE_DIR}/libs/zlib/windows/lib/zlibstatic.lib)
		target_include_directories(zlib PUBLIC ${CMAKE_SOURCE_DIR}/libs/zlib/windows/include)
	else()
		find_package(ZLIB)
		target_link_libraries(zlib PUBLIC ZLIB::ZLIB)
	endif()
endif()

target_link_libraries(compression
	PRIVATE
        zlib
        xz
	PUBLIC
		Qt6::Core
		qgc
)

target_include_directories(compression
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
)
