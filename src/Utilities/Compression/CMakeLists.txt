# ============================================================================
# Decompression Subsystem
# Core formats: ZIP, GZIP, XZ/LZMA, ZSTD (always enabled)
# Optional formats: LZ4, BZip2 (controlled by QGC_ENABLE_LZ4, QGC_ENABLE_BZIP2)
# ============================================================================

qt_add_library(QGCCompression STATIC
    QGCCompression.cc
    QGCCompression.h
    QGCCompressionTypes.h
    QGCCompressionJob.cc
    QGCCompressionJob.h
    QGCArchiveWatcher.cc
    QGCArchiveWatcher.h
    QGCArchiveModel.cc
    QGCArchiveModel.h
    QGClibarchive.cc
    QGClibarchive.h
    QGCArchiveDeviceBase.cc
    QGCArchiveDeviceBase.h
    QGCDecompressDevice.cc
    QGCDecompressDevice.h
    QGCArchiveFile.cc
    QGCArchiveFile.h
)

qt_add_qml_module(QGCCompression
    URI QGroundControl.Compression
    VERSION 1.0
    RESOURCE_PREFIX /qml
)

# Package NAMEs match the builtin CMake Find modules (ZLIB, LibLZMA, BZip2, LibArchive, ...)
# so QGC_USE_SYSTEM_LIBS resolves them via find_package (CPM checks ${NAME}_FOUND).
# <name>_ADDED = CPM built the static lib; otherwise a system copy provides the imported target.

# ============================================================================
# zlib Integration
# ============================================================================

set(ZLIB_EXTRA_OPTIONS)
if(WIN32)
    set(ZLIB_EXTRA_OPTIONS
        "ZLIB_INSTALL_COMPAT_DLL OFF"
    )
endif()

CPMAddPackage(
    NAME ZLIB
    GITHUB_REPOSITORY madler/zlib
    VERSION 1.3.2
    GIT_TAG v1.3.2
    OPTIONS
        "ZLIB_BUILD_TESTING OFF"
        "ZLIB_BUILD_SHARED OFF"
        "ZLIB_BUILD_STATIC ON"
        "ZLIB_INSTALL OFF"
        "ZLIB_PREFIX OFF"
        "${ZLIB_EXTRA_OPTIONS}"
)

if(ZLIB_ADDED)
    qgc_disable_dependency_warnings(zlibstatic)
    set(ZLIB_INCLUDE_DIR "${ZLIB_SOURCE_DIR};${ZLIB_BINARY_DIR}" CACHE PATH "" FORCE)
    set(ZLIB_LIBRARY "zlibstatic" CACHE STRING "" FORCE)
    set(ZLIB_LIBRARIES "zlibstatic" CACHE STRING "" FORCE)
    set(ZLIB_FOUND TRUE CACHE BOOL "" FORCE)
    if(NOT TARGET ZLIB::ZLIB)
        add_library(ZLIB::ZLIB ALIAS zlibstatic)
    endif()
elseif(NOT ZLIB_FOUND)
    message(FATAL_ERROR "QGC: ZLIB (required dependency) was not resolved")
endif()

# ============================================================================
# XZ (LZMA) Integration - liblzma from xz-utils
# ============================================================================

CPMAddPackage(
    NAME LibLZMA
    VERSION 5.8.3
    GITHUB_REPOSITORY tukaani-project/xz
    GIT_TAG v5.8.3
    OPTIONS
        "BUILD_TESTING OFF"
        "BUILD_SHARED_LIBS OFF"
        "XZ_TOOL_XZ OFF"
        "XZ_TOOL_XZDEC OFF"
        "XZ_TOOL_LZMADEC OFF"
        "XZ_TOOL_LZMAINFO OFF"
        "XZ_TOOL_SCRIPTS OFF"
        "XZ_NLS OFF"
        "XZ_THREADS no"
        "XZ_SMALL ON"
        "XZ_DOC OFF"
        "XZ_DOXYGEN OFF"
        "XZ_ENCODERS "
        "XZ_MICROLZMA_ENCODER OFF"
        "XZ_MICROLZMA_DECODER OFF"
        "XZ_LZIP_DECODER OFF"
)

if(LibLZMA_ADDED)
    if(NOT TARGET liblzma)
        message(FATAL_ERROR "liblzma target not found")
    endif()
    qgc_disable_dependency_warnings(liblzma)
    set(LIBLZMA_INCLUDE_DIR "${LibLZMA_SOURCE_DIR}/src/liblzma/api" CACHE PATH "" FORCE)
    set(LIBLZMA_INCLUDE_DIRS "${LibLZMA_SOURCE_DIR}/src/liblzma/api" CACHE PATH "" FORCE)
    set(LIBLZMA_LIBRARY "liblzma" CACHE STRING "" FORCE)
    set(LIBLZMA_FOUND TRUE CACHE BOOL "" FORCE)
    set(LIBLZMA_LIBRARIES "liblzma" CACHE STRING "" FORCE)
    set(LIBLZMA_HAS_AUTO_DECODER TRUE CACHE BOOL "" FORCE)
    set(LIBLZMA_HAS_EASY_ENCODER TRUE CACHE BOOL "" FORCE)
    set(LIBLZMA_HAS_LZMA_PRESET TRUE CACHE BOOL "" FORCE)
    set(LibLZMA_FOUND TRUE CACHE BOOL "" FORCE)
    # LZMA_API_STATIC tells lzma.h to drop the dllimport decoration; only correct
    # for the CPM static build, never for a system shared liblzma.
    set(LZMA_API_STATIC 1 CACHE INTERNAL "liblzma is static")
    if(NOT TARGET LibLZMA::LibLZMA)
        add_library(LibLZMA::LibLZMA ALIAS liblzma)
    endif()
elseif(NOT LibLZMA_FOUND)
    message(FATAL_ERROR "QGC: LibLZMA (required dependency) was not resolved")
endif()

set(HAVE_LZMA_STREAM_ENCODER_MT 0 CACHE INTERNAL "Encoder disabled in our build")
message(STATUS "liblzma support enabled")

# ============================================================================
# Zstandard (ZSTD) Integration
# ============================================================================

CPMAddPackage(
    NAME zstd
    VERSION 1.5.7
    GITHUB_REPOSITORY facebook/zstd
    GIT_TAG v1.5.7
    SOURCE_SUBDIR build/cmake
    OPTIONS
        "ZSTD_BUILD_PROGRAMS OFF"
        "ZSTD_BUILD_TESTS OFF"
        "ZSTD_BUILD_CONTRIB OFF"
        "ZSTD_BUILD_SHARED OFF"
        "ZSTD_BUILD_STATIC ON"
        "ZSTD_MULTITHREAD_SUPPORT OFF"
        "ZSTD_BUILD_COMPRESSION OFF"
        "ZSTD_BUILD_DICTBUILDER OFF"
)

# find_package(zstd) (config mode) may export zstd::libzstd_{static,shared}; map whichever
# exists onto the canonical Zstd::Zstd alias QGCCompression links.
if(zstd_ADDED)
    if(NOT TARGET libzstd_static)
        message(FATAL_ERROR "libzstd_static target not found")
    endif()
    qgc_disable_dependency_warnings(libzstd_static)
    set(ZSTD_INCLUDE_DIR "${zstd_SOURCE_DIR}/lib" CACHE PATH "" FORCE)
    set(ZSTD_LIBRARY "libzstd_static" CACHE STRING "" FORCE)
    set(ZSTD_LIBRARIES "libzstd_static" CACHE STRING "" FORCE)
    set(ZSTD_FOUND TRUE CACHE BOOL "" FORCE)
    set(Zstd_FOUND TRUE CACHE BOOL "" FORCE)
    if(NOT TARGET Zstd::Zstd)
        add_library(Zstd::Zstd ALIAS libzstd_static)
    endif()
    if(NOT TARGET zstd::libzstd_static)
        add_library(zstd::libzstd_static ALIAS libzstd_static)
    endif()
elseif(zstd_FOUND OR Zstd_FOUND)
    if(NOT TARGET Zstd::Zstd)
        if(TARGET zstd::libzstd_static)
            add_library(Zstd::Zstd ALIAS zstd::libzstd_static)
        elseif(TARGET zstd::libzstd_shared)
            add_library(Zstd::Zstd ALIAS zstd::libzstd_shared)
        endif()
    endif()
else()
    message(FATAL_ERROR "QGC: zstd (required dependency) was not resolved")
endif()

set(HAVE_LIBZSTD 1 CACHE INTERNAL "zstd decompression available")
set(HAVE_ZSTD_compressStream 0 CACHE INTERNAL "Compression disabled in our build")
set(HAVE_ZSTD_minCLevel 0 CACHE INTERNAL "Compression disabled in our build")
message(STATUS "Zstandard support enabled")

# ============================================================================
# LZ4 Integration (Fast compression) - Optional
# ============================================================================

if(QGC_ENABLE_LZ4)
    CPMAddPackage(
        NAME lz4
        VERSION 1.10.0
        GITHUB_REPOSITORY lz4/lz4
        GIT_TAG v1.10.0
        SOURCE_SUBDIR build/cmake
        OPTIONS
            "LZ4_BUILD_CLI OFF"
            "LZ4_BUILD_LEGACY_LZ4C OFF"
            "BUILD_SHARED_LIBS OFF"
            "BUILD_STATIC_LIBS ON"
    )

    # find_package(lz4) (config mode) may export LZ4::lz4.
    if(lz4_ADDED)
        qgc_disable_dependency_warnings(lz4_static)
        set(LZ4_INCLUDE_DIR "${lz4_SOURCE_DIR}/lib" CACHE PATH "" FORCE)
        set(LZ4_LIBRARY "lz4_static" CACHE STRING "" FORCE)
        set(LZ4_LIBRARIES "lz4_static" CACHE STRING "" FORCE)
        set(LZ4_FOUND TRUE CACHE BOOL "" FORCE)
        if(NOT TARGET LZ4::LZ4)
            add_library(LZ4::LZ4 ALIAS lz4_static)
        endif()
    elseif(lz4_FOUND)
        if(NOT TARGET LZ4::LZ4 AND TARGET LZ4::lz4)
            add_library(LZ4::LZ4 ALIAS LZ4::lz4)
        endif()
    else()
        message(FATAL_ERROR "QGC: lz4 (required dependency) was not resolved")
    endif()
    message(STATUS "LZ4 support enabled")
endif()

# ============================================================================
# BZip2 Integration (official repo with CMake support) - Optional
# ============================================================================

if(QGC_ENABLE_BZIP2)
    CPMAddPackage(
        NAME BZip2
        VERSION 1.0.8
        GIT_REPOSITORY https://gitlab.com/bzip2/bzip2.git
        GIT_TAG 66c46b8c9436613fd81bc5d03f63a61933a4dcc3
        OPTIONS
            "ENABLE_LIB_ONLY ON"
            "ENABLE_SHARED_LIB OFF"
            "ENABLE_STATIC_LIB ON"
            "ENABLE_APP OFF"
            "ENABLE_DOCS OFF"
            "ENABLE_EXAMPLES OFF"
            "ENABLE_TESTS OFF"
    )

    if(BZip2_ADDED)
        if(NOT TARGET bz2_static)
            message(FATAL_ERROR "bz2_static target not found")
        endif()
        qgc_disable_dependency_warnings(bz2_static)
        set(BZIP2_INCLUDE_DIR "${BZip2_SOURCE_DIR}" CACHE PATH "" FORCE)
        set(BZIP2_LIBRARY "bz2_static" CACHE STRING "" FORCE)
        set(BZIP2_FOUND TRUE CACHE BOOL "" FORCE)
        set(BZIP2_LIBRARIES "bz2_static" CACHE STRING "" FORCE)
        if(NOT TARGET BZip2::BZip2)
            add_library(BZip2::BZip2 ALIAS bz2_static)
        endif()
    elseif(NOT BZip2_FOUND)
        message(FATAL_ERROR "QGC: BZip2 (required dependency) was not resolved")
    endif()
    message(STATUS "BZip2 support enabled")
endif()

# ============================================================================
# libarchive Integration (Full-featured C library)
# ============================================================================

if(QGC_ENABLE_BZIP2)
    set(_LIBARCHIVE_BZIP2 "ON")
else()
    set(_LIBARCHIVE_BZIP2 "OFF")
endif()

if(QGC_ENABLE_LZ4)
    set(_LIBARCHIVE_LZ4 "ON")
else()
    set(_LIBARCHIVE_LZ4 "OFF")
endif()

CPMAddPackage(
    NAME LibArchive
    VERSION 3.8.8
    GITHUB_REPOSITORY libarchive/libarchive
    GIT_TAG v3.8.8
    OPTIONS
        "ENABLE_WERROR OFF"
        "ENABLE_TEST OFF"
        "ENABLE_INSTALL OFF"
        "ENABLE_TAR OFF"
        "ENABLE_CPIO OFF"
        "ENABLE_CAT OFF"
        "ENABLE_UNZIP OFF"
        "ENABLE_ACL OFF"
        "ENABLE_XATTR OFF"
        "ENABLE_ICONV OFF"
        "ENABLE_OPENSSL OFF"
        "ENABLE_MBEDTLS OFF"
        "ENABLE_NETTLE OFF"
        "ENABLE_CNG OFF"
        "ENABLE_LIBMD OFF"
        "ENABLE_LIBB2 OFF"
        "ENABLE_LIBXML2 OFF"
        "ENABLE_EXPAT OFF"
        "ENABLE_PCREPOSIX OFF"
        "ENABLE_PCRE2POSIX OFF"
        "POSIX_REGEX_LIB LIBC"
        "ENABLE_LZO OFF"
        "ENABLE_LIBGCC OFF"
        "ENABLE_ZLIB ON"
        "ENABLE_LZMA ON"
        "ENABLE_ZSTD ON"
        "ENABLE_BZip2 ${_LIBARCHIVE_BZIP2}"
        "ENABLE_LZ4 ${_LIBARCHIVE_LZ4}"
)

if(LibArchive_ADDED)
    if(NOT TARGET archive_static)
        message(FATAL_ERROR "libarchive target not found")
    endif()
    qgc_disable_dependency_warnings(archive_static)
    if(NOT TARGET LibArchive::LibArchive)
        add_library(LibArchive::LibArchive ALIAS archive_static)
    endif()
elseif(NOT LibArchive_FOUND)
    message(FATAL_ERROR "QGC: LibArchive (required dependency) was not resolved")
endif()
message(STATUS "libarchive support enabled")

target_link_libraries(QGCCompression
    PUBLIC
        Qt6::Core
        Qt6::Qml
        Qt6::QmlIntegration
        QGCFileSystem
    PRIVATE
        Qt6::Concurrent
        LibArchive::LibArchive
        ZLIB::ZLIB
        LibLZMA::LibLZMA
        Zstd::Zstd
        QGCLogging
)

target_include_directories(QGCCompression PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# libarchive's archive_random calls BCrypt* on Windows whenever HAVE_BCRYPT_H is set (independent
# of the disabled CNG crypto), and the static lib doesn't propagate bcrypt — link it here.
if(WIN32)
    target_link_libraries(QGCCompression PRIVATE bcrypt)
endif()

target_compile_definitions(QGCCompression PRIVATE LIBARCHIVE_STATIC)
if(QGC_ENABLE_BZIP2)
    target_compile_definitions(QGCCompression PUBLIC QGC_ENABLE_BZIP2)
endif()
if(QGC_ENABLE_LZ4)
    target_compile_definitions(QGCCompression PUBLIC QGC_ENABLE_LZ4)
endif()

target_precompile_headers(QGCCompression PRIVATE
    <QtCore/QObject>
    <QtCore/QString>
    <QtCore/QByteArray>
    <QtCore/QFile>
    <QtCore/QIODevice>
    <QtQmlIntegration/QtQmlIntegration>
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCCompression)
