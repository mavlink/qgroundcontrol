find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(compression STATIC
	QGCLZMA.cc
	QGCLZMA.h
	QGCZlib.cc
	QGCZlib.h
)

set(ZLIB_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
set(SKIP_INSTALL_ALL ON CACHE INTERNAL "")

include(FetchContent)
FetchContent_Declare(zlib
	GIT_REPOSITORY https://github.com/madler/zlib.git
	GIT_TAG v1.3.1
	GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(zlib)

target_link_libraries(compression
	PRIVATE
        zlib
        xz
	PUBLIC
		Qt6::Core
)

target_include_directories(compression PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
