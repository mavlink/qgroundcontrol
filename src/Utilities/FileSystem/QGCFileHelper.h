#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QCryptographicHash>
#include <QtCore/QString>
#include <QtCore/QTemporaryFile>
#include <QtCore/QUrl>

#include <memory>

/// Generic file system helper utilities
namespace QGCFileHelper {

// ============================================================================
// File Reading (with optional decompression)
// ============================================================================

/// Read file contents with optional automatic decompression
/// Supports transparent decompression of .gz, .xz, .zst, .bz2, .lz4 files
/// Uses QGCCompression::isCompressedFile() for format detection
/// @param filePath Path to file (may be compressed or uncompressed)
/// @param errorString Output error message on failure
/// @param maxBytes Maximum bytes to read (0 = unlimited, applies to decompressed size)
/// @return File contents (decompressed if applicable), empty on failure
QByteArray readFile(const QString &filePath, QString *errorString = nullptr,
                    qint64 maxBytes = 0);

// ============================================================================
// Buffer Configuration
// ============================================================================

/// Minimum buffer size for I/O operations
constexpr size_t kBufferSizeMin = 16384;     // 16KB

/// Maximum buffer size for I/O operations
constexpr size_t kBufferSizeMax = 131072;    // 128KB

/// Default buffer size when detection unavailable
constexpr size_t kBufferSizeDefault = 65536; // 64KB

/// Get optimal buffer size for I/O operations (platform-adaptive)
/// Uses filesystem block size when available, with reasonable fallback
/// @param path Optional path to query filesystem block size (empty = use root)
/// @return Buffer size in bytes (16KB-128KB range), cached after first call
size_t optimalBufferSize(const QString &path = QString());

// ============================================================================
// Path Utilities
// ============================================================================

/// Check if path exists (handles Qt resources which always "exist" if valid)
/// @param path File path or Qt resource path (:/...)
/// @return true if path exists
bool exists(const QString &path);

/// Join directory and filename with path separator
/// Handles trailing slashes correctly
/// @param dir Directory path
/// @param name File or subdirectory name
/// @return Combined path (dir + "/" + name)
QString joinPath(const QString &dir, const QString &name);

/// Ensure directory exists, creating it if necessary
/// @param path Directory path to ensure exists
/// @return true if directory exists or was created successfully
bool ensureDirectoryExists(const QString &path);

/// Ensure parent directory exists for a file path
/// @param filePath Path to a file (parent directory will be created if needed)
/// @return true if parent directory exists or was created successfully
bool ensureParentExists(const QString &filePath);

/// Copy a directory recursively to a new location
/// @param sourcePath Source directory path
/// @param destPath Destination directory path (created if doesn't exist)
/// @return true on success, false if any file/directory copy fails
bool copyDirectoryRecursively(const QString &sourcePath, const QString &destPath);

/// Move a file or directory, falling back to copy+delete for cross-filesystem moves
/// @param sourcePath Source path (file or directory)
/// @param destPath Destination path
/// @return true on success, false on failure
bool moveFileOrCopy(const QString &sourcePath, const QString &destPath);

// ============================================================================
// Safe File Operations
// ============================================================================

/// Write data to file atomically (prevents corruption on crash/power loss)
/// Uses QSaveFile internally: writes to temp file, then atomically renames.
/// @param filePath Target file path
/// @param data Data to write
/// @return true on success, false on any error (target unchanged on failure)
bool atomicWrite(const QString &filePath, const QByteArray &data);

// ============================================================================
// Disk Space Utilities
// ============================================================================

/// Check if there's sufficient disk space for an operation
/// @param path Target directory or file path
/// @param requiredBytes Bytes needed for the operation
/// @param margin Extra margin multiplier (default 1.1 = 10% safety margin)
/// @return true if sufficient space available, false otherwise
bool hasSufficientDiskSpace(const QString &path, qint64 requiredBytes, double margin = 1.1);

/// Get available disk space at a path
/// @param path Directory or file path to query
/// @return Available bytes, or -1 if unable to determine
qint64 availableDiskSpace(const QString &path);

// ============================================================================
// URL/Path Utilities
// ============================================================================

/// Convert URL or path string to local filesystem path
/// Handles: file:// URLs, qrc:// URLs (-> :/), and plain paths
/// @param urlOrPath URL string, QUrl, or filesystem path
/// @return Local path string, or original string if not convertible
QString toLocalPath(const QString &urlOrPath);

/// @overload Convert QUrl to local filesystem path
QString toLocalPath(const QUrl &url);

/// Check if URL or path refers to a local file (not network resource)
/// @param urlOrPath URL string or filesystem path
/// @return true if local file or Qt resource
bool isLocalPath(const QString &urlOrPath);

/// Check if path is a Qt resource path (:/ or qrc://)
/// @param path Path to check
/// @return true if Qt resource path
bool isQtResource(const QString &path);

// ============================================================================
// Checksum Utilities
// ============================================================================

/// Default hash algorithm for checksum operations
constexpr QCryptographicHash::Algorithm kDefaultHashAlgorithm = QCryptographicHash::Sha256;

/// Compute hash of file contents
/// @param filePath Path to file (supports Qt resources)
/// @param algorithm Hash algorithm (default: SHA-256)
/// @return Hash as hex string, or empty string on error
QString computeFileHash(const QString &filePath,
                        QCryptographicHash::Algorithm algorithm = kDefaultHashAlgorithm);

/// Compute hash of decompressed file contents
/// Automatically decompresses .gz, .xz, .zst, .bz2, .lz4 files before hashing
/// For uncompressed files, behaves identically to computeFileHash
/// @param filePath Path to file (may be compressed)
/// @param algorithm Hash algorithm (default: SHA-256)
/// @return Hash as hex string, or empty string on error
QString computeDecompressedFileHash(const QString &filePath,
                                    QCryptographicHash::Algorithm algorithm = kDefaultHashAlgorithm);

/// Compute hash of data
/// @param data Data to hash
/// @param algorithm Hash algorithm (default: SHA-256)
/// @return Hash as hex string
QString computeHash(const QByteArray &data,
                    QCryptographicHash::Algorithm algorithm = kDefaultHashAlgorithm);

/// Verify file matches expected hash
/// @param filePath Path to file
/// @param expectedHash Expected hash (hex string, case-insensitive)
/// @param algorithm Hash algorithm (default: SHA-256)
/// @return true if file hash matches expected hash
bool verifyFileHash(const QString &filePath, const QString &expectedHash,
                    QCryptographicHash::Algorithm algorithm = kDefaultHashAlgorithm);

/// Get human-readable name for hash algorithm
/// @param algorithm Hash algorithm
/// @return Name like "SHA-256", "MD5", etc.
QString hashAlgorithmName(QCryptographicHash::Algorithm algorithm);

// ============================================================================
// Temporary File Utilities
// ============================================================================

/// Create a temporary file with specified content
/// The file is automatically removed when the returned QTemporaryFile is destroyed
/// @param data Content to write to the temporary file
/// @param templateName Optional file name template (e.g., "myapp_XXXXXX.json")
/// @return Unique pointer to open temp file (positioned at start), or nullptr on error
std::unique_ptr<QTemporaryFile> createTempFile(const QByteArray &data,
                                                const QString &templateName = QString());

/// Create a temporary copy of an existing file
/// @param sourcePath Path to source file to copy
/// @param templateName Optional file name template (e.g., "backup_XXXXXX.dat")
/// @return Unique pointer to open temp file with copy, or nullptr on error
std::unique_ptr<QTemporaryFile> createTempCopy(const QString &sourcePath,
                                                const QString &templateName = QString());

/// Get the system temporary directory path
/// @return Path to system temp directory
QString tempDirectory();

/// Create a unique temporary file path without creating the file
/// Useful for operations that need a temp path but create the file themselves
/// @param templateName Optional file name template (e.g., "download_XXXXXX.tmp")
/// @return Unique temporary file path
QString uniqueTempPath(const QString &templateName = QString());

/// Safely replace a file using write-to-temp-then-rename pattern
/// More control than atomicWrite - uses provided temp file
/// @param tempFile Open temp file with new content (will be closed)
/// @param targetPath Final destination path
/// @param backupPath Optional path for backup of original (empty = no backup)
/// @return true on success, false on error (temp file removed on failure)
bool replaceFileFromTemp(QTemporaryFile *tempFile, const QString &targetPath,
                         const QString &backupPath = QString());

} // namespace QGCFileHelper
