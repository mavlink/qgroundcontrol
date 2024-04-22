find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(Compression STATIC
	QGCLZMA.cc
	QGCLZMA.h
	QGCZlib.cc
	QGCZlib.h
)

############### ZLIB

set(ZLIB_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
set(SKIP_INSTALL_FILES ON CACHE INTERNAL "")
set(SKIP_INSTALL_LIBRARIES ON CACHE INTERNAL "")

include(FetchContent)
FetchContent_Declare(zlib
	GIT_REPOSITORY https://github.com/madler/zlib.git
	GIT_TAG v1.3.1
	GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(zlib)

############### XZ

set(XZ_EMBEDDED_DIR ${CMAKE_SOURCE_DIR}/libs/xz-embedded)

qt_add_library(xz STATIC
    ${XZ_EMBEDDED_DIR}/linux/include/linux/xz.h
    ${XZ_EMBEDDED_DIR}/linux/lib/xz/xz_crc32.c
    ${XZ_EMBEDDED_DIR}/linux/lib/xz/xz_crc64.c
    ${XZ_EMBEDDED_DIR}/linux/lib/xz/xz_dec_lzma2.c
    ${XZ_EMBEDDED_DIR}/linux/lib/xz/xz_dec_stream.c
    ${XZ_EMBEDDED_DIR}/linux/lib/xz/xz_lzma2.h
    ${XZ_EMBEDDED_DIR}/linux/lib/xz/xz_private.h
    ${XZ_EMBEDDED_DIR}/linux/lib/xz/xz_stream.h
    ${XZ_EMBEDDED_DIR}/userspace/xz_config.h
)

target_include_directories(xz
	PUBLIC
        ${XZ_EMBEDDED_DIR}/linux/include/linux
    PRIVATE
        ${XZ_EMBEDDED_DIR}/linux/lib/xz
        ${XZ_EMBEDDED_DIR}/userspace
)

target_compile_definitions(xz
    PRIVATE
        XZ_DEC_ANY_CHECK
    PUBLIC
        XZ_USE_CRC64
)

###############

target_link_libraries(Compression
	PRIVATE
        zlibstatic
        xz
	PUBLIC
		Qt6::Core
)

target_include_directories(Compression PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
