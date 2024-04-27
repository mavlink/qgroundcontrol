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

FetchContent_Declare(xz
    GIT_REPOSITORY https://github.com/tukaani-project/xz-embedded.git
    GIT_TAG v2024-04-05
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(xz)

qt_add_library(xz STATIC
    ${xz_SOURCE_DIR}/linux/include/linux/xz.h
    ${xz_SOURCE_DIR}/linux/lib/xz/xz_crc32.c
    ${xz_SOURCE_DIR}/linux/lib/xz/xz_crc64.c
    ${xz_SOURCE_DIR}/linux/lib/xz/xz_dec_lzma2.c
    ${xz_SOURCE_DIR}/linux/lib/xz/xz_dec_stream.c
    ${xz_SOURCE_DIR}/linux/lib/xz/xz_lzma2.h
    ${xz_SOURCE_DIR}/linux/lib/xz/xz_private.h
    ${xz_SOURCE_DIR}/linux/lib/xz/xz_stream.h
    ${xz_SOURCE_DIR}/userspace/xz_config.h
)

target_include_directories(xz
	PUBLIC
        ${xz_SOURCE_DIR}/linux/include/linux
    PRIVATE
        ${xz_SOURCE_DIR}/linux/lib/xz
        ${xz_SOURCE_DIR}/userspace
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
