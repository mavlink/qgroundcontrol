target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        QGCLZMA.cc
        QGCLZMA.h
        QGCZip.cc
        QGCZip.h
        QGCZlib.cc
        QGCZlib.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

#===========================================================================#

set(ZLIB_EXTRA_OPTIONS)
if(WIN32)
    set(ZLIB_EXTRA_OPTIONS
        "ZLIB_INSTALL_COMPAT_DLL OFF"
    )
endif()

CPMAddPackage(
    NAME zlib
    GITHUB_REPOSITORY madler/zlib
    GIT_TAG develop
    OPTIONS
        "ZLIB_BUILD_TESTING OFF"
        "ZLIB_BUILD_SHARED OFF"
        "ZLIB_BUILD_STATIC ON"
        "ZLIB_BUILD_MINIZIP OFF"
        "ZLIB_INSTALL OFF"
        "ZLIB_PREFIX OFF"
        "${ZLIB_EXTRA_OPTIONS}"
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ZLIB::ZLIBSTATIC)

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

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE xz)
