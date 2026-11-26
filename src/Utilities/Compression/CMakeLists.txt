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
    PUBLIC
        Qt6::Core
        QGCFileSystem
    PRIVATE
        Qt6::Concurrent
        LibArchive::LibArchive
        ZLIB::ZLIB
        LibLZMA::LibLZMA
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

target_compile_definitions(QGCCompression PRIVATE LIBARCHIVE_STATIC)
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
