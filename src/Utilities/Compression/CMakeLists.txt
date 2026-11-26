# ============================================================================
# Decompression Subsystem
# Decompresses gzip and xz streams, and extracts ZIP archives. Legacy lzma, Zstandard, LZ4 and bzip2 streams, and 7z
# and TAR archives, if enabled
# ============================================================================

qt_add_library(
    QGCCompression STATIC
    Compression.cc
    Compression.h
    CompressionJob.cc
    CompressionJob.h
    CompressionTypes.h
    private/ArchiveExtraction.cc
    private/ArchiveExtraction.h
    private/ArchiveReader.cc
    private/ArchiveReader.h
    private/EntryData.cc
    private/EntryData.h
    private/EntryNames.cc
    private/EntryNames.h
    private/Formats.cc
    private/Formats.h
    private/GzipCheck.cc
    private/GzipCheck.h
    private/Operation.cc
    private/Operation.h
    private/StreamDecompression.cc
    private/StreamDecompression.h
)

# libarchive and its decoders
include(CompressionLibraries.cmake)

target_link_libraries(
    QGCCompression
    PUBLIC Qt6::Core
    PRIVATE
        Qt6::Concurrent
        LibArchive::LibArchive
        ZLIB::ZLIB
        LibLZMA::LibLZMA
        QGCFileSystem
        QGCLogging
)

# The headers in private/ include libarchive's headers, which only this library can find
target_include_directories(
    QGCCompression
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private
)

# libarchive's archive_random calls BCrypt* on Windows whenever HAVE_BCRYPT_H is set (independent
# of the disabled CNG crypto), and the static lib doesn't propagate bcrypt — link it here.
if(WIN32)
    target_link_libraries(QGCCompression PRIVATE bcrypt)
endif()

# A system libarchive (QGC_USE_SYSTEM_LIBS) is a shared library, whose functions Windows imports
if(LibArchive_ADDED)
    target_compile_definitions(QGCCompression PRIVATE LIBARCHIVE_STATIC)
endif()
if(QGC_ENABLE_BZIP2)
    target_compile_definitions(QGCCompression PUBLIC QGC_ENABLE_BZIP2)
endif()
if(QGC_ENABLE_LZ4)
    target_compile_definitions(QGCCompression PUBLIC QGC_ENABLE_LZ4)
endif()
if(QGC_ENABLE_ZSTD)
    target_compile_definitions(QGCCompression PUBLIC QGC_ENABLE_ZSTD)
    target_link_libraries(QGCCompression PRIVATE Zstd::Zstd)
endif()
# libarchive has no build switch for a format: what QGCCompression does not register is not linked
if(QGC_ENABLE_LZMA)
    target_compile_definitions(QGCCompression PUBLIC QGC_ENABLE_LZMA)
endif()
if(QGC_ENABLE_7ZIP)
    target_compile_definitions(QGCCompression PUBLIC QGC_ENABLE_7ZIP)
endif()
if(QGC_ENABLE_TAR)
    target_compile_definitions(QGCCompression PUBLIC QGC_ENABLE_TAR)
endif()

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCCompression)

# An ELF executable exports what it defines of a symbol that a shared library it links also uses. The libraries loaded
# with QGC (Qt, libpng, freetype, GStreamer plugins, ...) would then call the bundled zlib for some functions and the
# system's for the others. On Android the application is a shared library, which exports every symbol it defines,
# beside the system's libz.so, and keeps their code from --gc-sections. --exclude-libs keeps the symbols of the bundled
# archives inside each binary that links them, and the check fails a build of the application that exports them again
if(LINUX OR ANDROID)
    foreach(_qgc_archive IN ITEMS zlibstatic liblzma archive_static libzstd_static lz4_static bz2_static)
        if(TARGET ${_qgc_archive})
            get_target_property(_qgc_archive_imported ${_qgc_archive} IMPORTED)
            if(NOT _qgc_archive_imported)
                # One archive per option: mold 2.30 does not end when given a colon-separated list
                target_link_options(
                    QGCCompression INTERFACE "LINKER:--exclude-libs=$<TARGET_LINKER_FILE_NAME:${_qgc_archive}>"
                )
            endif()
        endif()
    endforeach()
    cmake_language(
        DEFER
        DIRECTORY
        "${CMAKE_SOURCE_DIR}"
        CALL
        add_custom_command
        TARGET
        ${CMAKE_PROJECT_NAME}
        POST_BUILD
        COMMAND
        "${CMAKE_COMMAND}"
        "-DAPP=$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
        "-DNM=${CMAKE_NM}"
        -P
        "${CMAKE_SOURCE_DIR}/cmake/BundledDecoderExports.cmake"
        COMMENT
        "Checking that the application exports no symbol of the bundled decompression libraries"
        VERBATIM
    )
endif()
