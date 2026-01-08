#pragma once

/// @file QGCFileWatcher.h
/// @brief Wrapper around QFileSystemWatcher with callback-based API

#include <QtCore/QFileSystemWatcher>
#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QTimer>

#include <functional>

/// Callback-based file/directory watcher with debouncing support
/// Provides a simpler API than QFileSystemWatcher for common use cases
///
/// Example usage:
/// @code
/// QGCFileWatcher watcher;
/// watcher.watchFile("/path/to/config.json", [](const QString &path) {
///     qDebug() << "Config changed:" << path;
/// });
/// @endcode
class QGCFileWatcher : public QObject
{
    Q_OBJECT

public:
    /// Callback for file/directory changes
    /// @param path Path to the changed file or directory
    using ChangeCallback = std::function<void(const QString &path)>;

    /// Construct a file watcher
    /// @param parent QObject parent
    explicit QGCFileWatcher(QObject *parent = nullptr);

    ~QGCFileWatcher() override;

    /// Set debounce delay for change notifications
    /// Multiple rapid changes are coalesced into a single callback
    /// @param milliseconds Debounce delay (default 100ms, 0 = no debounce)
    void setDebounceDelay(int milliseconds);

    /// Get current debounce delay
    /// @return Debounce delay in milliseconds
    int debounceDelay() const { return _debounceDelay; }

    // ========================================================================
    // File Watching
    // ========================================================================

    /// Watch a file for changes
    /// @param filePath Path to file to watch
    /// @param callback Function to call when file changes
    /// @return true if file was added to watch list
    bool watchFile(const QString &filePath, ChangeCallback callback);

    /// Watch a file for changes (signal-based)
    /// @param filePath Path to file to watch
    /// @return true if file was added to watch list
    bool watchFile(const QString &filePath);

    /// Stop watching a specific file
    /// @param filePath Path to file to stop watching
    /// @return true if file was being watched
    bool unwatchFile(const QString &filePath);

    /// Check if a file is being watched
    /// @param filePath Path to check
    /// @return true if file is being watched
    bool isWatchingFile(const QString &filePath) const;

    /// Get list of watched files
    /// @return List of file paths being watched
    QStringList watchedFiles() const;

    // ========================================================================
    // Directory Watching
    // ========================================================================

    /// Watch a directory for changes
    /// @param directoryPath Path to directory to watch
    /// @param callback Function to call when directory changes
    /// @return true if directory was added to watch list
    bool watchDirectory(const QString &directoryPath, ChangeCallback callback);

    /// Watch a directory for changes (signal-based)
    /// @param directoryPath Path to directory to watch
    /// @return true if directory was added to watch list
    bool watchDirectory(const QString &directoryPath);

    /// Stop watching a specific directory
    /// @param directoryPath Path to directory to stop watching
    /// @return true if directory was being watched
    bool unwatchDirectory(const QString &directoryPath);

    /// Check if a directory is being watched
    /// @param directoryPath Path to check
    /// @return true if directory is being watched
    bool isWatchingDirectory(const QString &directoryPath) const;

    /// Get list of watched directories
    /// @return List of directory paths being watched
    QStringList watchedDirectories() const;

    // ========================================================================
    // Bulk Operations
    // ========================================================================

    /// Watch multiple files
    /// @param filePaths List of file paths to watch
    /// @param callback Function to call when any file changes
    /// @return Number of files successfully added
    int watchFiles(const QStringList &filePaths, ChangeCallback callback);

    /// Watch multiple directories
    /// @param directoryPaths List of directory paths to watch
    /// @param callback Function to call when any directory changes
    /// @return Number of directories successfully added
    int watchDirectories(const QStringList &directoryPaths, ChangeCallback callback);

    /// Stop watching all files and directories
    void clear();

    // ========================================================================
    // Convenience Methods
    // ========================================================================

    /// Watch a file and automatically re-watch if it's recreated
    /// Useful for config files that may be replaced atomically
    /// @param filePath Path to file to watch
    /// @param callback Function to call when file changes
    /// @return true if file was added to watch list
    bool watchFilePersistent(const QString &filePath, ChangeCallback callback);

signals:
    /// Emitted when a watched file changes
    /// @param path Path to the changed file
    void fileChanged(const QString &path);

    /// Emitted when a watched directory changes
    /// @param path Path to the changed directory
    void directoryChanged(const QString &path);

private slots:
    void _onFileChanged(const QString &path);
    void _onDirectoryChanged(const QString &path);
    void _processPendingChanges();

private:
    void _scheduleCallback(const QString &path, bool isDirectory);

    QFileSystemWatcher *_watcher = nullptr;

    // Callbacks per path
    QHash<QString, ChangeCallback> _fileCallbacks;
    QHash<QString, ChangeCallback> _directoryCallbacks;

    // Persistent watch tracking
    QSet<QString> _persistentFiles;

    // Debouncing
    int _debounceDelay = 100;  // milliseconds
    QTimer *_debounceTimer = nullptr;
    QSet<QString> _pendingFileChanges;
    QSet<QString> _pendingDirectoryChanges;
};
