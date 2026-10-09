#pragma once

#include <chrono>
#include <cstdint>

#include <QtCore/QDeadlineTimer>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QString>

#include "GPSCancellation.h"
#include "GPSTransport.h"

namespace GPSTest {

/// Plays a raw receiver capture, such as a GPSRecordingTransport received file, back as a receiver link.
/// Writes are discarded but reported as written, and every baud rate is accepted, so configuration commands never
/// fail on I/O; replies come only from the capture. Reads return at most chunkBytes. A zero baud rate delivers the
/// capture as fast as it is read; otherwise bytes become available at that line rate, ten bits per byte, from open().
/// After the last byte the link stays idle, so reads time out without an error, and fatalError() reports it closed.
/// A developer tool, used by the hardware runner's --replay option and by tests.
class ReplayGPSTransport final : public GPSTransport
{
public:
    static constexpr int DEFAULT_CHUNK_BYTES = 1024;

    ReplayGPSTransport(QString path, GPSCancelToken cancelToken, unsigned baudrate = 0,
                       int chunkBytes = DEFAULT_CHUNK_BYTES);
    ~ReplayGPSTransport() override;

    GPSOpenResult open() override;
    /// True before open(), after a file error, and once the whole capture was delivered.
    bool fatalError() const override;
    GPSReadResult read(std::span<uint8_t> buffer, std::chrono::milliseconds timeout) override;
    bool setBaudrate(unsigned baudrate) override;

    /// The whole capture was delivered.
    [[nodiscard]] bool finished() const { return _finished; }

    [[nodiscard]] qint64 bytesDelivered() const { return _delivered; }

protected:
    GPSWriteResult writeData(QByteArrayView bytes, QDeadlineTimer deadline) override;

private:
    /// Bytes a read of @a length may take now.
    qint64 _available(qint64 length) const;
    /// When the next byte becomes available at the paced rate.
    QDeadlineTimer _nextByteDue() const;
    /// Sleeps until @a deadline unless cancelled first.
    void _sleepUntil(QDeadlineTimer deadline) const;

    QFile _file;
    const unsigned _baudrate;
    const int _chunkBytes;
    QElapsedTimer _clock;
    qint64 _size = 0;
    qint64 _delivered = 0;
    bool _finished = false;
    bool _failed = false;
};

}  // namespace GPSTest
