#pragma once

/// @file QGCCompressionTypes.h
/// @brief Shared type definitions for compression/archive operations

#include <QtCore/QDateTime>
#include <QtCore/QString>

#include <functional>

namespace QGCCompression {

// ============================================================================
// Progress Callback
// ============================================================================

/// Progress callback for long-running operations
/// @param bytesProcessed Bytes processed so far
/// @param totalBytes Total bytes to process (0 if unknown)
/// @return true to continue, false to cancel operation
using ProgressCallback = std::function<bool(qint64 bytesProcessed, qint64 totalBytes)>;

// ============================================================================
// Archive Entry Metadata
// ============================================================================

/// Default Unix permissions for extracted files (rw-r--r--)
constexpr quint32 kDefaultFilePermissions = 0644;

/// Metadata for a single entry in an archive
struct ArchiveEntry {
    QString name;                    ///< Path/name within archive
    qint64 size = 0;                 ///< Uncompressed size in bytes
    QDateTime modified;              ///< Last modification time
    bool isDirectory = false;        ///< True if entry is a directory
    quint32 permissions = kDefaultFilePermissions;  ///< Unix-style permissions
};

/// Summary statistics for an archive
struct ArchiveStats {
    int totalEntries = 0;            ///< Total number of entries (files + directories)
    int fileCount = 0;               ///< Number of files
    int directoryCount = 0;          ///< Number of directories
    qint64 totalUncompressedSize = 0;///< Sum of all file sizes (uncompressed)
    qint64 largestFileSize = 0;      ///< Size of largest file
    QString largestFileName;         ///< Name of largest file
};

/// Entry filter callback for selective extraction
/// Called for each entry before extraction; return true to extract, false to skip
/// @param entry Metadata for the archive entry
/// @return true to extract this entry, false to skip it
using EntryFilter = std::function<bool(const ArchiveEntry &entry)>;

} // namespace QGCCompression
