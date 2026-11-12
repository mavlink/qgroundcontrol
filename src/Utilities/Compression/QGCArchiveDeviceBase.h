#pragma once

/// @file QGCArchiveDeviceBase.h
/// @brief Base class for QIODevice wrappers using libarchive

#include <QtCore/QFile>
#include <QtCore/QIODevice>

#include <memory>

namespace QGClibarchive {
class ArchiveReader;
}

/// \brief Base class for QIODevice wrappers that use libarchive for decompression
///
/// Provides common source management, buffer handling, and archive lifecycle
/// Read-only, sequential access only
///
class QGCArchiveDeviceBase : public QIODevice
{
    Q_OBJECT

public:
    ~QGCArchiveDeviceBase() override;

    // QIODevice interface
    /// Open the device for reading: initArchive(), then prepareForReading()
    /// @param mode QIODevice::ReadOnly, the only mode there is
    bool open(OpenMode mode) override;
    void close() override;
    bool isSequential() const override { return true; }
    qint64 bytesAvailable() const override;

    /// Whether everything has been read: the stream has ended, or cannot be read any further
    /// Nothing is buffered before the first read, and between reads that took all there was, so this
    /// decompresses the next block to find out. A read error it runs into is in errorString().
    bool atEnd() const override;

    /// Why open() or a read failed
    /// The reason is also set as QIODevice's error string, for a caller that holds the device as a QIODevice:
    /// QIODevice::errorString() is not virtual, and returns "Unknown error" while nothing has failed.
    /// readAll() cannot fail. Of a truncated or corrupt stream it returns the part that could be decompressed,
    /// so check for an error after it. read() returns -1 once that part has been handed out.
    /// @return Reason of the failure, empty if nothing has failed since open() was last called
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
    /// Subclasses override to configure the formats and then call openArchive()
    /// @return true on success, sets _errorString on failure
    virtual bool initArchive() = 0;

    /// Called after archive is initialized, before reading begins
    /// Subclasses override to seek to entry, read headers, etc.
    /// @return true on success, sets _errorString on failure
    virtual bool prepareForReading() = 0;

    /// Check if ready to read data
    /// @return true if archive is initialized and ready
    virtual bool isReadyToRead() const = 0;

    /// Choose the formats openArchive() reads (called during initArchive)
    /// @param allFormats true for the archive formats (ZIP, TAR, 7z), false for raw (single-file) format
    void configureArchiveFormats(bool allFormats);

    /// Open the source, which is the file of the path constructor or the device of the other one, as an
    /// archive of the configured formats
    /// @return true on success, sets _errorString on failure
    bool openArchive();

    /// Read data from archive into internal buffer
    /// @return true if data was read, false on EOF or error
    bool fillBuffer();

    /// Take the format info the reader has recorded once the first header is read
    void captureFormatInfo();

    /// Record why open() or a read failed, for errorString() and QIODevice::errorString()
    void setError(const QString& reason);

    /// Reset all state (called by close())
    virtual void resetState();

    // Source management
    QString _filePath;
    QIODevice *_sourceDevice = nullptr;
    bool _ownsSource = false;
    std::unique_ptr<QIODevice> _ownedSource;

    // libarchive state
    std::unique_ptr<QGClibarchive::ArchiveReader> _reader;  // Null unless the archive is open
    bool _allFormats = false;
    bool _eof = false;

    // Decompressed data buffer
    QByteArray _buffer;

    // Error state
    QString _errorString;

    // Format info (populated after first header read)
    QString _formatName;
    QString _filterName;
};
