#pragma once

/// @file QGCArchiveWatcher.h
/// @brief Watches directories for archive files with optional auto-decompression

#include "QGCFileWatcher.h"
#include "QGCCompression.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QString>

Q_DECLARE_LOGGING_CATEGORY(QGCArchiveWatcherLog)

class QGCCompressionJob;

/// Watches directories for new archive files and optionally auto-decompresses them
///
/// Example usage:
/// @code
/// QGCArchiveWatcher watcher;
/// watcher.setAutoDecompress(true);
/// watcher.setOutputDirectory("/extracted");
/// connect(&watcher, &QGCArchiveWatcher::archiveDetected,
///         [](const QString &path) {
///     qDebug() << "Found archive:" << path;
/// });
/// connect(&watcher, &QGCArchiveWatcher::extractionComplete,
///         [](const QString &archive, const QString &output, bool success) {
///     qDebug() << "Extracted" << archive << "to" << output
///              << (success ? "OK" : "FAILED");
/// });
/// watcher.watchDirectory("/downloads");
/// @endcode
class QGCArchiveWatcher : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(QGCArchiveWatcher)

    /// Whether auto-decompression is enabled
    Q_PROPERTY(bool autoDecompress READ autoDecompress WRITE setAutoDecompress NOTIFY autoDecompressChanged FINAL)

    /// Whether an extraction is currently in progress
    Q_PROPERTY(bool extracting READ isExtracting NOTIFY extractingChanged FINAL)

    /// Current extraction progress (0.0 to 1.0)
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged FINAL)

    /// Output directory for extractions (empty = same directory as archive)
    Q_PROPERTY(QString outputDirectory READ outputDirectory WRITE setOutputDirectory
               NOTIFY outputDirectoryChanged FINAL)

public:
    /// Filter mode for which files to watch
    enum class FilterMode {
        Archives,           ///< Watch for archive files (.zip, .tar, .tar.gz, .7z)
        Compressed,         ///< Watch for single-file compressed (.gz, .xz, .zst, .bz2, .lz4)
        Both                ///< Watch for both archives and compressed files
    };
    Q_ENUM(FilterMode)

    explicit QGCArchiveWatcher(QObject *parent = nullptr);
    ~QGCArchiveWatcher() override;

    // ========================================================================
    // Configuration
    // ========================================================================

    /// Set filter mode for which file types to watch
    /// @param mode Filter mode
    void setFilterMode(FilterMode mode);
    FilterMode filterMode() const { return _filterMode; }

    /// Enable/disable automatic decompression of detected archives
    /// @param enable true to auto-decompress, false to just emit signals
    void setAutoDecompress(bool enable);
    bool autoDecompress() const { return _autoDecompress; }

    /// Set output directory for extracted files
    /// @param directory Output directory (empty = same directory as archive)
    void setOutputDirectory(const QString &directory);
    QString outputDirectory() const { return _outputDirectory; }

    /// Set whether to remove archives after successful extraction
    /// @param remove true to remove source archive after extraction
    void setRemoveAfterExtraction(bool remove);
    bool removeAfterExtraction() const { return _removeAfterExtraction; }

    /// Set debounce delay (inherited from QGCFileWatcher)
    /// @param milliseconds Debounce delay (default 500ms for archives)
    void setDebounceDelay(int milliseconds);
    int debounceDelay() const;

    // ========================================================================
    // Directory Watching
    // ========================================================================

    /// Watch a directory for archive files
    /// @param directoryPath Path to directory to watch
    /// @return true if directory was added to watch list
    bool watchDirectory(const QString &directoryPath);

    /// Stop watching a directory
    /// @param directoryPath Path to directory
    /// @return true if directory was being watched
    bool unwatchDirectory(const QString &directoryPath);

    /// Get list of watched directories
    /// @return List of directory paths being watched
    QStringList watchedDirectories() const;

    /// Stop watching all directories
    void clear();

    // ========================================================================
    // Status
    // ========================================================================

    /// Check if extraction is in progress
    bool isExtracting() const { return _extracting; }

    /// Get current extraction progress
    qreal progress() const { return _progress; }

    // ========================================================================
    // Manual Operations
    // ========================================================================

    /// Scan a directory for existing archives (useful for initial scan)
    /// @param directoryPath Directory to scan
    /// @return List of archive files found
    QStringList scanDirectory(const QString &directoryPath) const;

    /// Cancel current extraction
    void cancelExtraction();

signals:
    /// Emitted when an archive file is detected in a watched directory
    /// @param archivePath Full path to the archive file
    /// @param format Detected format
    void archiveDetected(const QString &archivePath, QGCCompression::Format format);

    /// Emitted when extraction completes (only if autoDecompress is enabled)
    /// @param archivePath Path to the source archive
    /// @param outputPath Path to extracted files/directory
    /// @param success true if extraction succeeded
    /// @param errorString Error message if failed
    void extractionComplete(const QString &archivePath, const QString &outputPath,
                            bool success, const QString &errorString);

    /// Property change signals
    void autoDecompressChanged(bool autoDecompress);
    void extractingChanged(bool extracting);
    void progressChanged(qreal progress);
    void outputDirectoryChanged(const QString &directory);

private slots:
    void _onDirectoryChanged(const QString &path);
    void _onExtractionProgress(qreal progress);
    void _onExtractionFinished(bool success);

private:
    bool _isWatchedFormat(const QString &filePath) const;
    void _processNewFile(const QString &filePath);
    void _startExtraction(const QString &archivePath);
    void _setExtracting(bool extracting);
    void _setProgress(qreal progress);

    QGCFileWatcher *_fileWatcher = nullptr;
    QGCCompressionJob *_extractionJob = nullptr;

    // Configuration
    FilterMode _filterMode = FilterMode::Both;
    bool _autoDecompress = false;
    bool _removeAfterExtraction = false;
    QString _outputDirectory;

    // State
    bool _extracting = false;
    qreal _progress = 0.0;

    // Track files to avoid duplicate processing
    QHash<QString, QSet<QString>> _knownFiles;  // directory -> known files

    // Pending extractions queue
    QStringList _pendingExtractions;
    QString _currentArchive;
};
