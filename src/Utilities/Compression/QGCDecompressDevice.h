#pragma once

/// @file QGCDecompressDevice.h
/// @brief QIODevice wrapper for streaming decompression

#include "QGCArchiveDeviceBase.h"

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(QGCDecompressDeviceLog)

/// QIODevice wrapper for streaming decompression of single-file formats
/// Supports .gz, .xz, .zst, .bz2, .lz4 compressed data
/// Read-only, sequential access only
///
/// Example usage:
/// @code
/// QGCDecompressDevice device("data.json.gz");
/// device.open(QIODevice::ReadOnly);
/// QTextStream stream(&device);
/// QString content = stream.readAll();
/// @endcode
class QGCDecompressDevice : public QGCArchiveDeviceBase
{
    Q_OBJECT

public:
    /// Construct from QIODevice source (streaming)
    /// @param source Compressed data source (must be open and readable)
    /// @param parent QObject parent
    /// @note The source device must remain valid until this device is closed
    explicit QGCDecompressDevice(QIODevice *source, QObject *parent = nullptr);

    /// Construct from file path
    /// @param filePath Path to compressed file (or Qt resource path :/)
    /// @param parent QObject parent
    explicit QGCDecompressDevice(const QString &filePath, QObject *parent = nullptr);

    ~QGCDecompressDevice() override = default;

    // QIODevice interface
    bool open(OpenMode mode) override;

protected:
    bool initArchive() override;
    bool prepareForReading() override;
    bool isReadyToRead() const override { return _headerRead; }
    void resetState() override;

private:
    bool _headerRead = false;
};
