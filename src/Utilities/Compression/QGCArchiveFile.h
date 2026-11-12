#pragma once

/// @file QGCArchiveFile.h
/// @brief QIODevice for reading a single entry from an archive

#include "QGCArchiveDeviceBase.h"

#include <QtCore/QDateTime>

/// \brief QIODevice for reading a single entry from an archive without full extraction
///
/// Supports ZIP, TAR, 7z, and other libarchive-supported formats
/// Read-only, sequential access only
/// A truncated or corrupt entry reads as far as it can be decompressed: errorString() tells of it
/// Only a regular file is read. open() fails for a directory, for a hard or symbolic link and for a special
/// file (FIFO, device node, socket), which would read as a file without data.
///
/// Example usage:
/// @code
/// QGCArchiveFile file("archive.zip", "config.json");
/// file.open(QIODevice::ReadOnly);
/// QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
/// @endcode
///
class QGCArchiveFile : public QGCArchiveDeviceBase
{
    Q_OBJECT

public:
    /// Construct from archive file path
    /// @param archivePath Path to archive file (or Qt resource path :/)
    /// @param entryName Name of entry to read within the archive
    /// @param parent QObject parent
    QGCArchiveFile(const QString &archivePath, const QString &entryName, QObject *parent = nullptr);

    /// Construct from QIODevice source (streaming)
    /// @param source Archive data source (must be open and readable)
    /// @param entryName Name of entry to read within the archive
    /// @param parent QObject parent
    /// @note The source device must remain valid until this device is closed
    QGCArchiveFile(QIODevice *source, const QString &entryName, QObject *parent = nullptr);

    ~QGCArchiveFile() override = default;

    // QIODevice interface
    qint64 size() const override;

    /// Get entry name being read
    /// @return Entry name within the archive
    QString entryName() const { return _entryName; }

    /// Get entry size (available after open)
    /// @return Entry size in bytes, or -1 if unknown
    qint64 entrySize() const { return _entrySize; }

    /// Check if entry was found in archive (available after open)
    /// @return true if entry exists and was found, and is a file that can be read
    bool entryFound() const { return _entryFound; }

    /// Get entry modification time (available after open)
    /// @return Modification time, or invalid QDateTime if unknown
    QDateTime entryModified() const { return _entryModified; }

protected:
    bool initArchive() override;
    bool prepareForReading() override;
    bool isReadyToRead() const override { return _entryFound; }
    void resetState() override;

private:
    bool seekToEntry();

    QString _entryName;
    bool _entryFound = false;
    qint64 _entrySize = -1;
    QDateTime _entryModified;
};
