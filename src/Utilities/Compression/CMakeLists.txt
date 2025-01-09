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
    FetchContent_Declare(zlib
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG "v${TARGET_ZLIB_VERSION}"
        GIT_SHALLOW TRUE
    )
    set(ZLIB_BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE)
    set(SKIP_INSTALL_FILES ON CACHE INTERNAL "" FORCE)
    set(SKIP_INSTALL_LIBRARIES ON CACHE INTERNAL "" FORCE)
    set(SKIP_INSTALL_HEADERS ON CACHE INTERNAL "" FORCE)
    set(SKIP_INSTALL_ALL ON CACHE INTERNAL "" FORCE)
    FetchContent_MakeAvailable(zlib)

    if(TARGET zlibstatic)
        target_link_libraries(Compression PRIVATE zlibstatic)
    endif()
endif()

#===========================================================================#

message(STATUS "Building XZ")
FetchContent_Declare(xz-embedded
    GIT_REPOSITORY https://github.com/tukaani-project/xz-embedded.git
    GIT_TAG v2024-12-30
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(xz-embedded)

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
