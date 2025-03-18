find_package(Qt6 REQUIRED COMPONENTS Core)

target_sources(QGC
    PRIVATE
        QGCLZMA.cc
        QGCLZMA.h
        QGCZip.cc
        QGCZip.h
        QGCZlib.cc
        QGCZlib.h
)

target_link_libraries(QGC
    PRIVATE
        Qt6::CorePrivate
    PUBLIC
        Qt6::Core
)

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#===========================================================================#

include(CPM)
CPMAddPackage(
    NAME zlib
    VERSION 1.3.1
    GITHUB_REPOSITORY madler/zlib
    OPTIONS
        "ZLIB_BUILD_EXAMPLES OFF"
        "SKIP_INSTALL_FILES ON"
        "SKIP_INSTALL_LIBRARIES ON"
        "SKIP_INSTALL_HEADERS ON"
        "SKIP_INSTALL_ALL ON"
)

if(zlib_ADDED)
    target_link_libraries(QGC PRIVATE zlibstatic)
else()
    find_package(Qt6ZlibPrivate REQUIRED)
    target_link_libraries(QGC PRIVATE Qt6::ZlibPrivate)
endif()

#===========================================================================#

CPMAddPackage(
    NAME xz-embedded
    VERSION 2024-12-30
    GITHUB_REPOSITORY tukaani-project/xz-embedded
    OPTIONS
        "BUILD_SHAPELIB_CONTRIB OFF"
        "BUILD_APPS OFF"
        "BUILD_TESTING OFF"
)

qt_add_library(xz STATIC
    ${xz-embedded_SOURCE_DIR}/linux/include/linux/xz.h
    ${xz-embedded_SOURCE_DIR}/linux/lib/xz/xz_crc32.c
    ${xz-embedded_SOURCE_DIR}/linux/lib/xz/xz_crc64.c
    ${xz-embedded_SOURCE_DIR}/linux/lib/xz/xz_dec_lzma2.c
    ${xz-embedded_SOURCE_DIR}/linux/lib/xz/xz_dec_stream.c
    ${xz-embedded_SOURCE_DIR}/linux/lib/xz/xz_lzma2.h
    ${xz-embedded_SOURCE_DIR}/linux/lib/xz/xz_private.h
    ${xz-embedded_SOURCE_DIR}/linux/lib/xz/xz_stream.h
    ${xz-embedded_SOURCE_DIR}/userspace/xz_config.h
)

target_include_directories(xz
    PUBLIC
        ${xz-embedded_SOURCE_DIR}/linux/include/linux
    PRIVATE
        ${xz-embedded_SOURCE_DIR}/linux/lib/xz
        ${xz-embedded_SOURCE_DIR}/userspace
)

target_compile_definitions(xz
    PRIVATE
        XZ_DEC_ANY_CHECK
    PUBLIC
        XZ_USE_CRC64
)

target_link_libraries(QGC PRIVATE xz)
