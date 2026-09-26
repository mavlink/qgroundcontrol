#pragma once

#include <chrono>
#include <cstdint>
#include <memory>

#include <QtCore/QDeadlineTimer>
#include <QtCore/QFile>
#include <QtCore/QString>

#include "GPSTransport.h"
#include "GPSType.h"

class QDateTime;

/// Tees a receiver link into two raw captures: the bytes read from the receiver, and the bytes the link accepted
/// for writing to it. The received file is the unmodified receiver stream, so a UBX capture opens in u-center.
/// Both files are created when the link first opens, on the thread that owns the transport, and written through
/// QFile's buffer, which is flushed every FLUSH_INTERVAL and on destruction.
/// Each file keeps its first maxFileBytes bytes and then stops growing. A capped diagnostic capture bounds disk use
/// without a retention policy or frames split across rotated files.
/// A file error logs one warning and ends the recording. The link and its results are never affected.
/// A developer diagnostic that the application never enables: wrap a transport with it, as the hardware runner's
/// --record option does. Recordings contain the receiver's position.
class GPSRecordingTransport final : public GPSTransport
{
public:
    struct Files
    {
        QString received;
        QString sent;
    };

    /// About six hours of a 115200 baud link at full load.
    static constexpr qint64 MAX_FILE_BYTES = qint64{256} * 1024 * 1024;
    static constexpr std::chrono::milliseconds FLUSH_INTERVAL{1000};

    GPSRecordingTransport(std::unique_ptr<GPSTransport> transport, Files files, qint64 maxFileBytes = MAX_FILE_BYTES);
    ~GPSRecordingTransport() override;

    /// Unused capture paths in @a directory for a session of @a type that starts at @a startedAt:
    /// gps-<family>-<yyyyMMdd-HHmmss>-rx.<receivedExtension()> and gps-<family>-<yyyyMMdd-HHmmss>-tx.bin. When an
    /// earlier session already used that name, -2, -3, ... follow the time.
    [[nodiscard]] static Files sessionFiles(const QString& directory, GPSType type, const QDateTime& startedAt);
    /// ubx for u-blox, sbf for Septentrio, nmea for the NMEA-framed families, and bin for the others.
    [[nodiscard]] static QString receivedExtension(GPSType type);

    GPSOpenResult open() override;
    bool fatalError() const override;
    unsigned fixedBaudrate() const override;
    GPSReadResult read(uint8_t* buffer, int length, std::chrono::milliseconds timeout) override;
    std::chrono::milliseconds configurationWriteTimeout() const override;
    bool setBaudrate(unsigned baudrate) override;

    /// Either file is still being written.
    [[nodiscard]] bool recording() const;

protected:
    GPSWriteResult writeData(const uint8_t* buffer, int length, QDeadlineTimer deadline) override;

private:
    struct Capture
    {
        QFile file;
        qint64 bytes = 0;
        bool active = false;
    };

    void _start();
    void _record(Capture& capture, const uint8_t* data, qint64 length);
    void _flushIfDue();
    /// Ends both captures after a file error; the first error is the only one reported.
    void _fail(const QString& path, const QString& error);
    void _finish(Capture& capture);

    std::unique_ptr<GPSTransport> _transport;
    const qint64 _maxFileBytes;
    Capture _received;
    Capture _sent;
    QDeadlineTimer _nextFlush;
    bool _started = false;
};
