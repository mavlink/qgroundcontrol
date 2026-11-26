# ============================================================================
# The libraries QGCCompression uses: libarchive, and the decoders it is built with
# Always required: zlib (gzip, ZIP) and liblzma (xz; legacy lzma with QGC_ENABLE_LZMA, 7z with QGC_ENABLE_7ZIP)
# Optional, OFF by default: Zstandard, LZ4, bzip2 (QGC_ENABLE_ZSTD, QGC_ENABLE_LZ4, QGC_ENABLE_BZIP2)
# ============================================================================

include_guard(GLOBAL)

# Variables handed to the bundled libraries keep the names their find modules expect.
# cmake-lint: disable=C0103

# Package names match the builtin CMake Find modules (ZLIB, LibLZMA, BZip2, LibArchive) or the CMake packages the
# libraries install (zstd, lz4). <name>_ADDED = CPM built the static lib; otherwise a system copy provides the imported
# target.

# With QGC_USE_SYSTEM_LIBS or QGC_SYSTEM_LIBS_ONLY, looks for an installed <package> of at least <minimum>, the oldest
# release with what QGCCompression and libarchive use, and sets QGC_SYSTEM_<package> when it finds one; the caller then
# skips its CPMAddPackage(). CPM's own find_package() asks for the pinned version, newer than the copies of current
# distributions. First clears what qgc_compression_publish() cached for a bundled copy in this build directory, which
# the find module would take for an installed one.
macro(qgc_compression_find_system package minimum)
    set(QGC_SYSTEM_${package} FALSE)
    if(QGC_USE_SYSTEM_LIBS OR QGC_SYSTEM_LIBS_ONLY)
        string(TOUPPER "${package}" _qgc_prefix)
        if(QGC_COMPRESSION_BUNDLED_${_qgc_prefix})
            foreach(_qgc_suffix IN ITEMS INCLUDE_DIR INCLUDE_DIRS LIBRARY LIBRARIES FOUND)
                unset(${_qgc_prefix}_${_qgc_suffix} CACHE)
            endforeach()
            unset(${package}_FOUND CACHE)
            unset(LZMA_API_STATIC CACHE)
            unset(QGC_COMPRESSION_BUNDLED_${_qgc_prefix} CACHE)
        endif()
        get_directory_property(_qgc_targets_before IMPORTED_TARGETS)
        find_package(${package} ${minimum} QUIET)
        if(${package}_FOUND)
            set(QGC_SYSTEM_${package} TRUE)
            # find_package() makes its targets for this directory alone; the tests link them by name from theirs
            get_directory_property(_qgc_targets IMPORTED_TARGETS)
            if(_qgc_targets_before)
                list(REMOVE_ITEM _qgc_targets ${_qgc_targets_before})
            endif()
            foreach(_qgc_target IN LISTS _qgc_targets)
                set_target_properties(
                    ${_qgc_target}
                    PROPERTIES
                        IMPORTED_GLOBAL TRUE
                )
            endforeach()
        elseif(QGC_SYSTEM_LIBS_ONLY)
            message(
                FATAL_ERROR
                    "QGC: QGC_SYSTEM_LIBS_ONLY is set, but no installed ${package} ${minimum} or newer was found"
            )
        endif()
    endif()
endmacro()

# Says whether the build uses the installed or the bundled copy of <package>
function(qgc_compression_report package)
    string(TOUPPER "${package}" _prefix)
    if(QGC_SYSTEM_${package})
        set(_version "${${package}_VERSION}")
        if(NOT _version)
            set(_version "${${_prefix}_VERSION}")
        endif()
        message(STATUS "QGC: ${package}: the installed ${_version} (QGC_USE_SYSTEM_LIBS)")
    elseif(${package}_ADDED)
        message(STATUS "QGC: ${package}: the bundled ${CPM_PACKAGE_${package}_VERSION}")
    endif()
endfunction()

# Publishes a static library that CPM built: sets the cache entries its find module would set, which
# libarchive's find_package() calls then use, and adds the imported target name that QGCCompression links.
function(qgc_compression_publish prefix target include_dir alias)
    qgc_disable_dependency_warnings(${target})
    set(QGC_COMPRESSION_BUNDLED_${prefix}
        TRUE
        CACHE INTERNAL "QGCCompression bundles this library"
    )
    set(${prefix}_INCLUDE_DIR
        "${include_dir}"
        CACHE PATH "" FORCE
    )
    set(${prefix}_LIBRARY
        "${target}"
        CACHE STRING "" FORCE
    )
    set(${prefix}_LIBRARIES
        "${target}"
        CACHE STRING "" FORCE
    )
    set(${prefix}_FOUND
        TRUE
        CACHE BOOL "" FORCE
    )
    if(NOT TARGET ${alias})
        add_library(${alias} ALIAS ${target})
    endif()
endfunction()

# ============================================================================
# zlib Integration
# ============================================================================

# libarchive asks for 1.2.1; GzipCheck inflates gzip (windowBits 16 + MAX_WBITS), which zlib reads since 1.2.0
qgc_compression_find_system(ZLIB 1.2.1)
if(NOT QGC_SYSTEM_ZLIB)
    CPMAddPackage(
        NAME ZLIB
        GITHUB_REPOSITORY madler/zlib
        VERSION 1.4
        GIT_TAG v1.4
        OPTIONS
            "ZLIB_BUILD_TESTING OFF"
            "ZLIB_BUILD_SHARED OFF"
            "ZLIB_BUILD_STATIC ON"
            "ZLIB_INSTALL OFF"
    )
endif()

if(ZLIB_ADDED)
    qgc_compression_publish(ZLIB zlibstatic "${ZLIB_SOURCE_DIR};${ZLIB_BINARY_DIR}" ZLIB::ZLIB)
elseif(NOT ZLIB_FOUND)
    message(FATAL_ERROR "QGC: ZLIB (required dependency) was not resolved")
endif()
qgc_compression_report(ZLIB)

# ============================================================================
# XZ (LZMA) Integration - liblzma from xz-utils
# ============================================================================

# 5.0.0 is the first stable liblzma API, which libarchive's xz, lzma and 7z readers call
qgc_compression_find_system(LibLZMA 5.0.0)
# The releases of xz that carry the backdoor (CVE-2024-3094)
if(QGC_SYSTEM_LibLZMA AND LIBLZMA_VERSION MATCHES "^5\\.6\\.[01]$")
    message(FATAL_ERROR "QGC: the installed liblzma is ${LIBLZMA_VERSION}, a release with a backdoor (CVE-2024-3094)")
endif()
if(NOT QGC_SYSTEM_LibLZMA)
    CPMAddPackage(
        NAME LibLZMA
        VERSION 5.8.4
        GITHUB_REPOSITORY tukaani-project/xz
        GIT_TAG v5.8.4
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
endif()

if(LibLZMA_ADDED)
    if(NOT TARGET liblzma)
        message(FATAL_ERROR "liblzma target not found")
    endif()
    qgc_compression_publish(LIBLZMA liblzma "${LibLZMA_SOURCE_DIR}/src/liblzma/api" LibLZMA::LibLZMA)
    set(LIBLZMA_INCLUDE_DIRS
        "${LibLZMA_SOURCE_DIR}/src/liblzma/api"
        CACHE PATH "" FORCE
    )
    set(LIBLZMA_HAS_AUTO_DECODER
        TRUE
        CACHE BOOL "" FORCE
    )
    set(LIBLZMA_HAS_EASY_ENCODER
        TRUE
        CACHE BOOL "" FORCE
    )
    set(LIBLZMA_HAS_LZMA_PRESET
        TRUE
        CACHE BOOL "" FORCE
    )
    set(LibLZMA_FOUND
        TRUE
        CACHE BOOL "" FORCE
    )
    # LZMA_API_STATIC tells lzma.h to drop the dllimport decoration; only correct
    # for the CPM static build, never for a system shared liblzma.
    set(LZMA_API_STATIC
        1
        CACHE INTERNAL "liblzma is static"
    )
elseif(NOT LibLZMA_FOUND)
    message(FATAL_ERROR "QGC: LibLZMA (required dependency) was not resolved")
endif()

set(HAVE_LZMA_STREAM_ENCODER_MT
    0
    CACHE INTERNAL "Encoder disabled in our build"
)
qgc_compression_report(LibLZMA)

# ============================================================================
# Zstandard (ZSTD) Integration - Optional
# ============================================================================

if(QGC_ENABLE_ZSTD)
    # 1.0.0 fixed the Zstandard frame format and the streaming API libarchive's reader calls
    qgc_compression_find_system(zstd 1.0.0)
    if(NOT QGC_SYSTEM_zstd)
        CPMAddPackage(
            NAME zstd
            VERSION 1.6.0
            GITHUB_REPOSITORY facebook/zstd
            GIT_TAG v1.6.0
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
                # Frame formats of zstd before 1.0: more decoders exposed to hostile input, and nothing writes them
                "ZSTD_LEGACY_SUPPORT OFF"
        )
    endif()

    # find_package(zstd) (config mode) may export zstd::libzstd_{static,shared}; map whichever
    # exists onto the canonical Zstd::Zstd alias QGCCompression links.
    if(zstd_ADDED)
        if(NOT TARGET libzstd_static)
            message(FATAL_ERROR "libzstd_static target not found")
        endif()
        qgc_compression_publish(ZSTD libzstd_static "${zstd_SOURCE_DIR}/lib" Zstd::Zstd)
        set(Zstd_FOUND
            TRUE
            CACHE BOOL "" FORCE
        )
        if(NOT TARGET zstd::libzstd_static)
            add_library(zstd::libzstd_static ALIAS libzstd_static)
        endif()
    elseif(zstd_FOUND OR Zstd_FOUND)
        # The shared library, which libarchive finds as well: a second, static copy would be linked beside it
        if(NOT TARGET Zstd::Zstd)
            if(TARGET zstd::libzstd_shared)
                add_library(Zstd::Zstd ALIAS zstd::libzstd_shared)
            elseif(TARGET zstd::libzstd_static)
                add_library(Zstd::Zstd ALIAS zstd::libzstd_static)
            endif()
        endif()
    else()
        message(FATAL_ERROR "QGC: zstd (required dependency) was not resolved")
    endif()

    set(HAVE_LIBZSTD
        1
        CACHE INTERNAL "zstd decompression available"
    )
    set(HAVE_ZSTD_compressStream
        0
        CACHE INTERNAL "Compression disabled in our build"
    )
    set(HAVE_ZSTD_minCLevel
        0
        CACHE INTERNAL "Compression disabled in our build"
    )
    qgc_compression_report(zstd)
else()
    # A build directory that was once configured with zstd keeps the cache entries published then, and
    # libarchive would take them as a zstd to link
    foreach(
        _qgc_zstd_cache_entry IN
        ITEMS
            ZSTD_INCLUDE_DIR
            ZSTD_LIBRARY
            ZSTD_LIBRARIES
            ZSTD_FOUND
            Zstd_FOUND
            HAVE_LIBZSTD
            HAVE_ZSTD_compressStream
            HAVE_ZSTD_minCLevel
    )
        unset(${_qgc_zstd_cache_entry} CACHE)
    endforeach()
endif()

# ============================================================================
# LZ4 Integration - Optional
# ============================================================================

if(QGC_ENABLE_LZ4)
    # libarchive's LZ4 reader decodes linked blocks with LZ4_decompress_safe_usingDict from 1.7.0 on
    qgc_compression_find_system(lz4 1.7.0)
    if(NOT QGC_SYSTEM_lz4)
        CPMAddPackage(
            NAME lz4
            VERSION 1.11.1
            GITHUB_REPOSITORY lz4/lz4
            GIT_TAG v1.11.1
            SOURCE_SUBDIR build/cmake
            OPTIONS
                "LZ4_BUILD_CLI OFF"
                "LZ4_BUILD_LEGACY_LZ4C OFF"
                "BUILD_SHARED_LIBS OFF"
                "BUILD_STATIC_LIBS ON"
        )
    endif()

    # find_package(lz4) (config mode) may export LZ4::lz4.
    if(lz4_ADDED)
        qgc_compression_publish(LZ4 lz4_static "${lz4_SOURCE_DIR}/lib" LZ4::LZ4)
    elseif(lz4_FOUND)
        if(NOT TARGET LZ4::LZ4 AND TARGET LZ4::lz4)
            add_library(LZ4::LZ4 ALIAS LZ4::lz4)
        endif()
    else()
        message(FATAL_ERROR "QGC: lz4 (required dependency) was not resolved")
    endif()
    qgc_compression_report(lz4)
else()
    # libarchive caches that it found this header of LZ4, and would include it in a build directory that had LZ4
    unset(HAVE_LZ4HC_H CACHE)
endif()

# ============================================================================
# BZip2 Integration (official repo with CMake support) - Optional
# ============================================================================

if(QGC_ENABLE_BZIP2)
    # libarchive calls the BZ2_ prefixed functions of bzip2 1.0
    qgc_compression_find_system(BZip2 1.0.0)
    if(NOT QGC_SYSTEM_BZip2)
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
    endif()

    if(BZip2_ADDED)
        if(NOT TARGET bz2_static)
            message(FATAL_ERROR "bz2_static target not found")
        endif()
        qgc_compression_publish(BZIP2 bz2_static "${BZip2_SOURCE_DIR}" BZip2::BZip2)
    elseif(NOT BZip2_FOUND)
        message(FATAL_ERROR "QGC: BZip2 (required dependency) was not resolved")
    endif()
    qgc_compression_report(BZip2)
endif()

# ============================================================================
# libarchive Integration
# ============================================================================

# ArchiveExtraction reads archive_entry_hardlink_is_set(), which libarchive 3.7.5 added
qgc_compression_find_system(LibArchive 3.7.5)
if(NOT QGC_SYSTEM_LibArchive)
    CPMAddPackage(
        NAME LibArchive
        VERSION 3.8.11
        GITHUB_REPOSITORY libarchive/libarchive
        GIT_TAG v3.8.11
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
            "ENABLE_MD OFF"
            "ENABLE_LIBB2 OFF"
            "ENABLE_LIBXML2 OFF"
            "ENABLE_EXPAT OFF"
            "ENABLE_BSDXML OFF"
            "ENABLE_WIN32_XMLLITE OFF"
            "ENABLE_PCREPOSIX OFF"
            "ENABLE_PCRE2POSIX OFF"
            "POSIX_REGEX_LIB LIBC"
            "ENABLE_LZO OFF"
            "ENABLE_LIBGCC OFF"
            "ENABLE_ZLIB ON"
            "ENABLE_LZMA ON"
            "ENABLE_ZSTD ${QGC_ENABLE_ZSTD}"
            "ENABLE_BZip2 ${QGC_ENABLE_BZIP2}"
            "ENABLE_LZ4 ${QGC_ENABLE_LZ4}"
    )
endif()

if(LibArchive_ADDED)
    if(NOT TARGET archive_static)
        message(FATAL_ERROR "libarchive target not found")
    endif()
    qgc_disable_dependency_warnings(archive_static)
    if(NOT TARGET LibArchive::LibArchive)
        add_library(LibArchive::LibArchive ALIAS archive_static)
    endif()
    # gzip and xz are read in every build. libarchive builds without a library it does not find, and would then
    # decode the format with an external program, which QGCCompression never registers
    foreach(_qgc_decoder IN ITEMS HAVE_LIBZ HAVE_LIBLZMA)
        get_directory_property(_qgc_has_decoder DIRECTORY "${LibArchive_SOURCE_DIR}" DEFINITION ${_qgc_decoder})
        if(NOT _qgc_has_decoder)
            message(FATAL_ERROR "QGC: libarchive was configured without ${_qgc_decoder}, so it cannot read gzip or xz")
        endif()
    endforeach()
elseif(NOT LibArchive_FOUND)
    message(FATAL_ERROR "QGC: LibArchive (required dependency) was not resolved")
else()
    include(${CMAKE_SOURCE_DIR}/cmake/modules/LibArchiveDecoders.cmake)
    qgc_check_libarchive_decoders(LibArchive::LibArchive ZLIB::ZLIB LibLZMA::LibLZMA)
endif()
qgc_compression_report(LibArchive)

# ============================================================================
# Licences of the bundled libraries
# ============================================================================

# The application carries what it links of them. The licences of libarchive, Zstandard, LZ4 and bzip2 ask a binary
# distribution to reproduce them; zlib's and liblzma's go along
if(LibArchive_ADDED)
    qgc_install_license(libarchive "${LibArchive_SOURCE_DIR}/COPYING")
endif()
if(ZLIB_ADDED)
    qgc_install_license(zlib "${ZLIB_SOURCE_DIR}/LICENSE")
endif()
if(LibLZMA_ADDED)
    qgc_install_license(xz "${LibLZMA_SOURCE_DIR}/COPYING" "${LibLZMA_SOURCE_DIR}/COPYING.0BSD")
endif()
if(zstd_ADDED)
    qgc_install_license(zstd "${zstd_SOURCE_DIR}/LICENSE")
endif()
if(lz4_ADDED)
    qgc_install_license(lz4 "${lz4_SOURCE_DIR}/lib/LICENSE")
endif()
if(BZip2_ADDED)
    qgc_install_license(bzip2 "${BZip2_SOURCE_DIR}/COPYING")
endif()
