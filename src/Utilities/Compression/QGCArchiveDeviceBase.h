#pragma once

/// @file QGCArchiveDeviceBase.h
/// @brief Base class for QIODevice wrappers using libarchive

#include <QtCore/QBuffer>
#include <QtCore/QFile>
#include <QtCore/QIODevice>

#include <memory>

struct archive;

/// Base class for QIODevice wrappers that use libarchive for decompression
/// Provides common source management, buffer handling, and archive lifecycle
/// Read-only, sequential access only
class QGCArchiveDeviceBase : public QIODevice
{
    Q_OBJECT

public:
    ~QGCArchiveDeviceBase() override;

    // QIODevice interface
    void close() override;
    bool isSequential() const override { return true; }
    qint64 bytesAvailable() const override;

    /// Get error description
    /// @return Error string if error occurred, empty otherwise
    QString errorString() const;

    /// Get detected archive format name (available after open)
    /// @return Format name like "RAW", "ZIP 2.0", "POSIX ustar", or empty if not detected
    QString formatName() const { return _formatName; }

    /// Get detected compression filter name (available after open)
    /// @return Filter name like "gzip", "xz", "zstd", "none", or empty if not detected
    QString filterName() const { return _filterName; }

protected:
    /// Construct with file path
    /// @param filePath Path to file (or Qt resource path :/)
    /// @param parent QObject parent
    explicit QGCArchiveDeviceBase(const QString &filePath, QObject *parent = nullptr);

    /// Construct with QIODevice source
    /// @param source Data source (must be open and readable)
    /// @param parent QObject parent
    /// @note The source device must remain valid until this device is closed
    explicit QGCArchiveDeviceBase(QIODevice *source, QObject *parent = nullptr);

    // QIODevice interface
    qint64 readData(char *data, qint64 maxSize) override;
    qint64 writeData(const char *data, qint64 maxSize) override;

    /// Initialize source device from file path
    /// Opens file or Qt resource and sets up _sourceDevice
    /// @return true on success, sets _errorString on failure
    bool initSourceFromPath();

    /// Initialize libarchive reader with configured format support
    /// Subclasses must override to configure format support before calling base
    /// @return true on success, sets _errorString on failure
    virtual bool initArchive() = 0;

    /// Called after archive is initialized, before reading begins
    /// Subclasses override to seek to entry, read headers, etc.
    /// @return true on success, sets _errorString on failure
    virtual bool prepareForReading() = 0;

    /// Check if ready to read data
    /// @return true if archive is initialized and ready
    virtual bool isReadyToRead() const = 0;

    /// Configure libarchive format support (called during initArchive)
    /// @param allFormats true for all archive formats, false for raw (single-file) format
    void configureArchiveFormats(bool allFormats);

    /// Open the configured archive for reading
    /// @return true on success, sets _errorString on failure
    bool openArchive();

    /// Read data from archive into internal buffer
    /// @return true if data was read, false on EOF or error
    bool fillBuffer();

    /// Capture format info from the archive after first header read
    void captureFormatInfo();

    /// Reset all state (called by close())
    virtual void resetState();

    // Source management
    QString _filePath;
    QIODevice *_sourceDevice = nullptr;
    bool _ownsSource = false;
    std::unique_ptr<QIODevice> _ownedSource;

    // libarchive state
    struct archive *_archive = nullptr;
    QByteArray _resourceData;  // For Qt resources (must outlive archive)
    bool _eof = false;

    // Decompressed data buffer
    QByteArray _buffer;

    // Error state
    QString _errorString;

    // Format info (populated after first header read)
    QString _formatName;
    QString _filterName;
};
