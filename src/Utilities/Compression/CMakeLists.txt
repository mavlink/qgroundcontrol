# ============================================================================
# Decompression Subsystem
# Unified decompression using libarchive (decompression-only)
# Core formats: ZIP, GZIP, XZ/LZMA, ZSTD (always enabled)
# Optional formats: LZ4, BZip2 (controlled by QGC_ENABLE_LZ4, QGC_ENABLE_BZIP2)
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        # Unified decompression interface
        QGCCompression.cc
        QGCCompression.h
        QGCCompressionTypes.h
        # Async job wrapper (QObject with signals)
        QGCCompressionJob.cc
        QGCCompressionJob.h
        # QML model for archive contents
        QGCArchiveModel.cc
        QGCArchiveModel.h
        # libarchive-based implementation (private)
        QGClibarchive.cc
        QGClibarchive.h
        # QIODevice streaming wrappers
        QGCArchiveDeviceBase.cc
        QGCArchiveDeviceBase.h
        QGCDecompressDevice.cc
        QGCDecompressDevice.h
        QGCArchiveFile.cc
        QGCArchiveFile.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ============================================================================
# zlib Integration
# ============================================================================

# Platform-specific zlib options
set(ZLIB_EXTRA_OPTIONS)
if(WIN32)
    set(ZLIB_EXTRA_OPTIONS
        "ZLIB_INSTALL_COMPAT_DLL OFF"
    )
endif()

CPMAddPackage(
    NAME zlib
    GITHUB_REPOSITORY madler/zlib
    VERSION 1.3.1.2
    GIT_TAG v1.3.1.2
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

# Export ZLIB paths for libarchive to find
set(ZLIB_INCLUDE_DIR "${zlib_SOURCE_DIR};${zlib_BINARY_DIR}" CACHE PATH "" FORCE)
set(ZLIB_LIBRARY "zlibstatic" CACHE STRING "" FORCE)
set(ZLIB_LIBRARIES "zlibstatic" CACHE STRING "" FORCE)
set(ZLIB_FOUND TRUE CACHE BOOL "" FORCE)
if(NOT TARGET ZLIB::ZLIB)
    add_library(ZLIB::ZLIB ALIAS zlibstatic)
endif()

# ============================================================================
# XZ (LZMA) Integration - liblzma from xz-utils
# ============================================================================

CPMAddPackage(
    NAME xz
    VERSION 5.8.2
    GITHUB_REPOSITORY tukaani-project/xz
    GIT_TAG v5.8.2
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
        "XZ_SMALL ON"                    # Smaller code size (slightly slower)
        "XZ_DOC OFF"
        "XZ_DOXYGEN OFF"
        "XZ_ENCODERS "                   # Decoder-only: disable all encoders
        "XZ_MICROLZMA_ENCODER OFF"       # Not used by QGC
        "XZ_MICROLZMA_DECODER OFF"       # Not used by QGC
        "XZ_LZIP_DECODER OFF"            # Not used by QGC
)

if(TARGET liblzma)
    # Set Find-module variables so libarchive's find_package(LibLZMA) succeeds
    set(LIBLZMA_INCLUDE_DIR "${xz_SOURCE_DIR}/src/liblzma/api" CACHE PATH "" FORCE)
    set(LIBLZMA_INCLUDE_DIRS "${xz_SOURCE_DIR}/src/liblzma/api" CACHE PATH "" FORCE)  # libarchive uses plural
    set(LIBLZMA_LIBRARY "liblzma" CACHE STRING "" FORCE)
    set(LIBLZMA_FOUND TRUE CACHE BOOL "" FORCE)
    set(LIBLZMA_LIBRARIES "liblzma" CACHE STRING "" FORCE)
    set(LIBLZMA_HAS_AUTO_DECODER TRUE CACHE BOOL "" FORCE)
    set(LIBLZMA_HAS_EASY_ENCODER TRUE CACHE BOOL "" FORCE)   # Satisfy FindLibLZMA sanity check
    set(LIBLZMA_HAS_LZMA_PRESET TRUE CACHE BOOL "" FORCE)   # Satisfy FindLibLZMA sanity check
    set(LibLZMA_FOUND TRUE CACHE BOOL "" FORCE)

    # Pre-cache configure-time check results (checks require built library, but we only have target)
    # These values match our decoder-only build configuration
    set(LZMA_API_STATIC 1 CACHE INTERNAL "liblzma is static")
    set(HAVE_LZMA_STREAM_ENCODER_MT 0 CACHE INTERNAL "Encoder disabled in our build")

    if(NOT TARGET LibLZMA::LibLZMA)
        add_library(LibLZMA::LibLZMA ALIAS liblzma)
    endif()
    message(STATUS "liblzma support enabled")
else()
    message(FATAL_ERROR "liblzma target not found")
endif()

# ============================================================================
# Zstandard (ZSTD) Integration
# ============================================================================

# Suppress CMake warning about MSVC not being an ASM assembler (CMP0194)
if(POLICY CMP0194)
    set(CMAKE_POLICY_DEFAULT_CMP0194 NEW CACHE STRING "" FORCE)
endif()

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
        "ZSTD_BUILD_COMPRESSION OFF"     # Decoder-only: QGC only decompresses
        "ZSTD_BUILD_DICTBUILDER OFF"     # Dictionary builder not needed
)

if(TARGET libzstd_static)
    # Help libarchive find our ZSTD (prevent using system zstd during cross-compile)
    set(ZSTD_INCLUDE_DIR "${zstd_SOURCE_DIR}/lib" CACHE PATH "" FORCE)
    set(ZSTD_LIBRARY "libzstd_static" CACHE STRING "" FORCE)
    set(ZSTD_LIBRARIES "libzstd_static" CACHE STRING "" FORCE)
    set(ZSTD_FOUND TRUE CACHE BOOL "" FORCE)
    set(Zstd_FOUND TRUE CACHE BOOL "" FORCE)

    # Pre-cache configure-time check results (checks require built library, but we only have target)
    # These values match our decoder-only build configuration
    set(HAVE_LIBZSTD 1 CACHE INTERNAL "zstd decompression available")
    set(HAVE_ZSTD_compressStream 0 CACHE INTERNAL "Compression disabled in our build")
    set(HAVE_ZSTD_minCLevel 0 CACHE INTERNAL "Compression disabled in our build")

    # Create the imported targets that consumers expect
    if(NOT TARGET Zstd::Zstd)
        add_library(Zstd::Zstd ALIAS libzstd_static)
    endif()
    if(NOT TARGET zstd::libzstd_static)
        add_library(zstd::libzstd_static ALIAS libzstd_static)
    endif()
    message(STATUS "Zstandard support enabled")
else()
    message(FATAL_ERROR "libzstd_static target not found")
endif()

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

    if(TARGET lz4_static)
        # Help libarchive find our LZ4
        set(LZ4_INCLUDE_DIR "${lz4_SOURCE_DIR}/lib" CACHE PATH "" FORCE)
        set(LZ4_LIBRARY "lz4_static" CACHE STRING "" FORCE)
        set(LZ4_LIBRARIES "lz4_static" CACHE STRING "" FORCE)
        set(LZ4_FOUND TRUE CACHE BOOL "" FORCE)
        if(NOT TARGET LZ4::LZ4)
            add_library(LZ4::LZ4 ALIAS lz4_static)
        endif()
        message(STATUS "LZ4 support enabled")
    else()
        message(FATAL_ERROR "lz4_static target not found")
    endif()
endif()

# ============================================================================
# BZip2 Integration (official repo with CMake support) - Optional
# ============================================================================
# NOTE: Using commit from master branch because:
# - Official releases (1.0.8 and earlier) do not include CMakeLists.txt
# - CMake support was added post-1.0.8 and no new release has been made
# - Risk: Commit may become unavailable if repo history is rewritten
# - TODO: Update to tagged release when bzip2 publishes one with CMake support

if(QGC_ENABLE_BZIP2)
    CPMAddPackage(
        NAME bzip2
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

    if(TARGET bz2_static)
        # Help libarchive/minizip find our BZip2
        set(BZIP2_INCLUDE_DIR "${bzip2_SOURCE_DIR}" CACHE PATH "" FORCE)
        set(BZIP2_LIBRARY "bz2_static" CACHE STRING "" FORCE)
        set(BZIP2_FOUND TRUE CACHE BOOL "" FORCE)
        set(BZIP2_LIBRARIES "bz2_static" CACHE STRING "" FORCE)
        if(NOT TARGET BZip2::BZip2)
            add_library(BZip2::BZip2 ALIAS bz2_static)
        endif()
        message(STATUS "BZip2 support enabled")
    else()
        message(FATAL_ERROR "bz2_static target not found")
    endif()
endif()

# ============================================================================
# libarchive Integration (Full-featured C library)
# ============================================================================

# Configure optional format support based on QGC options
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
    NAME libarchive
    VERSION 3.8.4
    GITHUB_REPOSITORY libarchive/libarchive
    GIT_TAG v3.8.4
    OPTIONS
        "ENABLE_WERROR OFF"
        "ENABLE_TEST OFF"
        "ENABLE_INSTALL OFF"
        # Disable CLI tools
        "ENABLE_TAR OFF"
        "ENABLE_CPIO OFF"
        "ENABLE_CAT OFF"
        "ENABLE_UNZIP OFF"
        # Disable unused features
        "ENABLE_ACL OFF"
        "ENABLE_XATTR OFF"
        "ENABLE_ICONV OFF"
        # Disable crypto (not needed for decompression)
        "ENABLE_OPENSSL OFF"
        "ENABLE_MBEDTLS OFF"
        "ENABLE_NETTLE OFF"
        "ENABLE_CNG OFF"
        "ENABLE_LIBMD OFF"
        "ENABLE_LIBB2 OFF"
        # Disable XML/regex (not needed for decompression)
        "ENABLE_LIBXML2 OFF"
        "ENABLE_EXPAT OFF"
        "ENABLE_PCREPOSIX OFF"
        "ENABLE_PCRE2POSIX OFF"
        # Skip PCREPOSIX block entirely (has bug: ENABLE_LIBGCC OFF causes fatal error)
        "POSIX_REGEX_LIB LIBC"
        # Disable unused compression formats
        "ENABLE_LZO OFF"
        "ENABLE_LIBGCC OFF"
        # Enable required formats
        "ENABLE_ZLIB ON"
        "ENABLE_LZMA ON"
        "ENABLE_ZSTD ON"
        "ENABLE_BZip2 ${_LIBARCHIVE_BZIP2}"
        "ENABLE_LZ4 ${_LIBARCHIVE_LZ4}"
)

if(TARGET archive_static)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE archive_static)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE LIBARCHIVE_STATIC)
    if(QGC_ENABLE_BZIP2)
        target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_ENABLE_BZIP2)
    endif()
    if(QGC_ENABLE_LZ4)
        target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_ENABLE_LZ4)
    endif()
    message(STATUS "libarchive support enabled")
else()
    message(FATAL_ERROR "libarchive target not found")
endif()


