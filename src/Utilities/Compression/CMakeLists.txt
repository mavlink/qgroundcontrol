find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(Compression STATIC
    QGCLZMA.cc
    QGCLZMA.h
    QGCZip.cc
    QGCZip.h
    QGCZlib.cc
    QGCZlib.h
)

target_link_libraries(Compression
    PRIVATE
        Qt6::CorePrivate
        Utilities
    PUBLIC
        Qt6::Core
)

target_include_directories(Compression PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#===========================================================================#

set(MINIMUM_ZLIB_VERSION 1.2.13)
set(TARGET_ZLIB_VERSION 1.3.1)

if(NOT QGC_BUILD_DEPENDENCIES)
    set(ZLIB_USE_STATIC_LIBS ON)
    find_package(ZLIB ${MINIMUM_ZLIB_VERSION})
    if(TARGET ZLIB::ZLIB)
        target_link_libraries(Compression PRIVATE ZLIB::ZLIB)
    endif()
endif()

if(NOT TARGET ZLIB::ZLIB)
    message(STATUS "Building ZLIB")

    include(CPM)
    CPMAddPackage(
        NAME zlib
        VERSION ${TARGET_ZLIB_VERSION}
        GITHUB_REPOSITORY madler/zlib
        OPTIONS
            "ZLIB_BUILD_EXAMPLES OFF"
            "SKIP_INSTALL_FILES ON"
            "SKIP_INSTALL_LIBRARIES ON"
            "SKIP_INSTALL_HEADERS ON"
            "SKIP_INSTALL_ALL ON"
    )

    if(TARGET zlibstatic)
        target_link_libraries(Compression PRIVATE zlibstatic)
    endif()
endif()

#===========================================================================#

message(STATUS "Building XZ")

include(CPM)
CPMAddPackage(
    NAME xz-embedded
    GITHUB_REPOSITORY tukaani-project/xz-embedded
    GIT_TAG "v2024-12-30"
    GIT_SHALLOW TRUE
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

target_link_libraries(Compression PRIVATE xz)
