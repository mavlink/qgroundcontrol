find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(compression STATIC
	QGCLZMA.cc
	QGCLZMA.h
	QGCZlib.cc
	QGCZlib.h
)

target_link_libraries(compression
	PRIVATE
        zlib
        xz
	PUBLIC
		Qt6::Core
)

target_include_directories(compression PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
