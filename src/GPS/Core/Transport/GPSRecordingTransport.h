#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QString>

#include "GPSBoundedFile.h"
#include "GPSClock.h"
#include "GPSReceiverConfig.h"
#include "GPSTransport.h"
#include "GPSType.h"

class QDateTime;

/// Records a receiver session: every call passes through to the link it wraps, and what crossed the link, or was
/// refused by it, goes to a text file, which the test tooling reads (GPSRecordingReader) and replays
/// (ReplayGPSTransport), also as a dialogue that checks what a driver sends (GPSDialogueTransport).
/// The file starts with FILE_HEADER on a line of its own. Every other line is one record, in the order the session
/// made the calls: the microseconds from the start of the recording to the return of the call, on the session's
/// clock, then one of
/// - RX and the bytes read from the receiver, in hexadecimal;
/// - TX and the bytes the link accepted for writing to the receiver, in hexadecimal;
/// - BAUD and the baud rate the link changed to;
/// - TX-REFUSED, how the link answered a write of which it accepted nothing (refusalName()), and the bytes of that
///   write, in hexadecimal. A write that the session makes after the deadline it gives it is such a write: it is
///   answered timed-out before the link is given its bytes;
/// - BAUD-REFUSED and a baud rate the link did not change to.
/// A session goes on after either refusal, which is why they are recorded. A call that the session's own
/// cancellation ended is not, nor is the rest of a write of which the link accepted the start: neither session goes on.
/// The recording of a session that was described to the recorder holds one more record, before those: SETTINGS and
/// what the session was asked to do (settingsRecord()), without which its commands cannot be told from another
/// session's.
/// The file is a GPSBoundedFile, created when the link first opens, on the thread that owns the transport, and flushed
/// every FLUSH_INTERVAL and on destruction. It keeps the records that fit in maxFileBytes and then stops growing,
/// which bounds disk use without a retention policy.
/// A file error logs one warning and ends the recording. The link and its results are never affected.
/// A recording that ends before its session, after a file error or at the size limit, calls the stopped callback
/// once, on the thread that uses the transport.
/// Recordings contain the receiver's position.
class GPSRecordingTransport final : public GPSTransport
{
public:
    /// The first line of a recording; the number is the format version.
    static constexpr char FILE_HEADER[] = "QGC GPS receiver recording 1";
    static constexpr char RECEIVED[] = "RX";
    static constexpr char SENT[] = "TX";
    static constexpr char BAUD[] = "BAUD";
    static constexpr char SENT_REFUSED[] = "TX-REFUSED";
    static constexpr char BAUD_REFUSED[] = "BAUD-REFUSED";
    static constexpr char SETTINGS[] = "SETTINGS";

    /// What a session was asked to do, which decides the commands it sends.
    struct Session
    {
        GPSType type = GPSType::Automatic;
        GPSReceiverConfig config{};
    };

    /// About three hours of a 115200 baud link at full load.
    static constexpr qint64 MAX_FILE_BYTES = qint64{256} * 1024 * 1024;
    static constexpr std::chrono::milliseconds FLUSH_INTERVAL{1000};

    /// The recording holds the settings of @a session, when one is given, and its times are those of @a clock, the
    /// clock the session runs on: the steady clock unless a test injects one.
    GPSRecordingTransport(std::unique_ptr<GPSTransport> transport, const QString& file,
                          qint64 maxFileBytes = MAX_FILE_BYTES, std::function<void()> stopped = {},
                          std::optional<Session> session = {}, GPSClock clock = {});
    ~GPSRecordingTransport() override;

    /// The value of the SETTINGS record of @a session on a link with the fixed baud rate @a fixedBaudRate
    /// (GPSTransport::fixedBaudRate()): "key=value" pairs separated by commas, numbers as short as they read back
    /// exactly. Always type (the receiver type in lower case, as in the file name), baud (the selected rate, zero for
    /// detection), fixed-baud, save and compact (0 or 1: consent to persistent changes, compact observations) and
    /// base, which the settings of the base mode follow:
    /// - survey: duration (seconds) and accuracy (metres);
    /// - fixed: latitude and longitude (degrees), altitude and accuracy (metres);
    /// - averaging: duration (seconds).
    /// Before base, and only where a base leaves constellations out of its corrections: constellations (the
    /// GPSCorrectionConstellation bits of those it sends besides GPS).
    [[nodiscard]] static QByteArray settingsRecord(const Session& session, unsigned fixedBaudRate);

    /// How a TX-REFUSED record names @a answer, the status of a write of which the link accepted nothing: completed,
    /// timed-out, error or unsupported. Null for a cancelled write, which the session's end refused, not the link.
    [[nodiscard]] static const char* refusalName(GPSWriteStatus answer);

    /// The recording in @a directory of a session of @a type that starts at @a startedAt:
    /// <yyyyMMdd>T<HHmmss.zzz>Z-<type>.log, in UTC and with the type in lower case. The name does not keep two
    /// sessions apart that start in the same millisecond, and the recorder does not write over a file: a caller takes
    /// @a startedAt from GPSFileStartTimes, which gives each of its sessions a time of its own.
    [[nodiscard]] static QString sessionFile(const QString& directory, GPSType type, const QDateTime& startedAt);

    GPSOpenResult open() override;
    bool fatalError() const override;
    unsigned fixedBaudRate() const override;
    GPSReadResult read(std::span<uint8_t> buffer, std::chrono::milliseconds timeout) override;
    std::chrono::milliseconds configurationWriteTimeout() const override;
    bool setBaudRate(unsigned baudRate) override;

    /// The file is still being written.
    [[nodiscard]] bool recording() const { return _file.isOpen(); }

protected:
    GPSWriteResult writeData(QByteArrayView bytes, QDeadlineTimer deadline) override;
    GPSWriteResult writeExpired(QByteArrayView bytes) override;

private:
    void _start();
    void _record(const char* kind, const QByteArray& value);
    void _write(const QByteArray& line);
    void _flushIfDue();
    void _notifyStopped();

    std::unique_ptr<GPSTransport> _transport;
    const QString _path;
    std::function<void()> _stopped;
    const std::optional<Session> _session;
    const GPSClock _clock;
    GPSBoundedFile _file;
    uint64_t _startedAtUs = 0;
    QDeadlineTimer _nextFlush;
    bool _started = false;
};
