#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>

#include <QtCore/QByteArrayView>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QFile>
#include <QtCore/QString>

#include "GPSCancellation.h"
#include "GPSRecordingReader.h"
#include "GPSTransport.h"

namespace GPSTest {

class GPSTestClock;

/// Where a replayed session left its recording.
struct GPSDialogueDivergence
{
    /// The line of the record in the recording that the session did not reproduce.
    qint64 line = 0;
    /// That record with its time, such as "TX 8 bytes b5620a0400000e34 at 0.004211 s" or "BAUD-REFUSED 921600 at
    /// 0.000102 s".
    QString recorded;
    /// What the driver did in its place, with the time of the replay.
    QString driver;

    /// "line <line>: the recording has <recorded>; the driver <driver>".
    [[nodiscard]] QString text() const;
};

/// Replays a session recording (GPSRecordingTransport) as the dialogue it was: the link of a driver that is expected
/// to repeat the recorded session, and the check that it does. The driver runs on @a clock, a virtual one, which the
/// link moves as the recorded session's clock moved:
/// - A read returns the bytes of the next RX record, one recorded read at a time, once the clock has reached the
///   record's time or the read's timeout reaches it, give or take READ_LATENESS. Any other read times out and moves
///   the clock by its timeout: a recording holds no read that returned nothing, so the driver's own timeouts stand
///   in for them. While the next record is anything else the recorded session acted before it read more, so reads
///   time out.
/// - The bytes the driver writes must be those of the TX records that follow, in order. Only the bytes count, not how
///   the write calls divide them.
/// - A write of the bytes of the next TX-REFUSED record gets the answer of the recorded link, which accepted none of
///   them. That link refused one write, so here the call counts: it must hold those bytes and no others.
/// - A baud rate the driver sets must be that of the next BAUD record, or of the next BAUD-REFUSED record, which
///   refuses it as the recorded link did and leaves the link working.
/// - Reproducing a record moves the clock up to the record's time.
/// The first thing the driver does that the recording does not have is the divergence(), and so is a record other
/// than an RX that the driver leaves unwritten for MISSING_WRITE_LIMIT. The link then reports cancellation, which ends
/// the driver's session without a failure. It does so too once the driver goes on after the last record: a recording
/// does not say what its session did after that.
/// A recording made before refusals were recorded holds neither kind. Its link refuses any rate but its fixed one, as
/// that link did. A session that met another refusal diverges where it did.
class GPSDialogueTransport final : public GPSTransport
{
public:
    /// How long after the end of a read's timeout the read may have returned the data of an RX record. A record
    /// holds the time its call returned, and a link returns a little after the timeout it was given; a driver that
    /// then found its deadline passed would otherwise act before the data it had when it was recorded. Shorter than
    /// any read slice or command timeout of the drivers.
    static constexpr std::chrono::milliseconds READ_LATENESS{2};
    /// A session that has not written a recorded TX or TX-REFUSED, or set a recorded BAUD or BAUD-REFUSED, this long
    /// after the record's time and after the record before it never will: the receiver worker ends a configured
    /// session after as long without useful data, and no driver waits longer before it writes.
    static constexpr std::chrono::seconds MISSING_WRITE_LIMIT{30};

    GPSDialogueTransport(const QString& recording, GPSTestClock& clock);
    ~GPSDialogueTransport() override;

    /// The settings of the recorded session, known once the link is open; nothing for a recording without them.
    [[nodiscard]] const std::optional<GPSRecordingSettings>& settings() const { return _reader.settings(); }

    /// Fails for a file that cannot be read or is not a session recording. The recording starts at the clock's time.
    GPSOpenResult open() override;
    /// True before open() and for a damaged recording.
    bool fatalError() const override;
    /// That of the recorded link; zero, as of a serial port, for a recording without settings.
    unsigned fixedBaudRate() const override;
    GPSReadResult read(std::span<uint8_t> buffer, std::chrono::milliseconds timeout) override;
    bool setBaudRate(unsigned baudRate) override;

    /// The driver's session ended, for @a reason: a divergence while records remain.
    void sessionEnded(const QString& reason);

    /// Ends the replay, as an interruption does.
    void cancel() { (void) _stop.cancel(); }

    /// Every record was reproduced.
    [[nodiscard]] bool finished() const;

    [[nodiscard]] const std::optional<GPSDialogueDivergence>& divergence() const { return _divergence; }

    /// Where the recording is damaged; empty otherwise.
    [[nodiscard]] const QString& error() const { return _error; }

    /// The records the session reproduced.
    [[nodiscard]] int records() const { return _records; }

    [[nodiscard]] qint64 receivedBytes() const { return _receivedBytes; }

    /// Without the writes the link refused.
    [[nodiscard]] qint64 sentBytes() const { return _sentBytes; }

    /// Without the rates the link refused.
    [[nodiscard]] int baudRateChanges() const { return _baudRateChanges; }

protected:
    GPSWriteResult writeData(QByteArrayView bytes, QDeadlineTimer deadline) override;

    /// Time is the clock's here: the real time the driver lost before it wrote makes no write late, and the recording
    /// answers it.
    GPSWriteResult writeExpired(QByteArrayView bytes) override
    {
        return writeData(bytes, QDeadlineTimer(QDeadlineTimer::Forever));
    }

private:
    GPSDialogueTransport(const QString& recording, GPSTestClock& clock, GPSCancelSource stop);

    /// When the next record is due on the clock.
    uint64_t _dueUs() const;
    /// The next record was reproduced.
    void _reproduced();
    /// Loads the next record, or none at the end of the recording and at damage.
    void _load();
    /// The driver did @a action where the recording has the next record.
    void _diverge(const QString& action);
    QString _describeRecord() const;
    QString _replayTime() const;

    GPSCancelSource _stop;
    QFile _file;
    GPSRecordingReader _reader;
    GPSTestClock& _clock;
    uint64_t _startUs = 0;
    /// When the record before the next one was reproduced.
    uint64_t _progressUs = 0;
    /// The record the session has to reproduce next.
    std::optional<GPSRecordingRecord> _record;
    /// The bytes of it that were read or written already.
    qsizetype _offset = 0;
    std::optional<GPSDialogueDivergence> _divergence;
    QString _error;
    int _records = 0;
    qint64 _receivedBytes = 0;
    qint64 _sentBytes = 0;
    int _baudRateChanges = 0;
};

/// What replaying a recording as a dialogue found.
struct GPSDialogueResult
{
    /// Why the dialogue was not replayed to its end: a file that is not a session recording, is damaged or does not
    /// hold its session's settings, or an interruption. Empty otherwise.
    QString error;
    std::optional<GPSDialogueDivergence> divergence;
    /// The session the driver ran as.
    std::optional<GPSRecordingSettings> settings;
    /// GPSDriver::configure() succeeded. It fails when the recording ends before the receiver is configured.
    bool configured = false;
    QString configurationError;
    QString receiverIdentity;
    /// The family the driver of a GPSType::Automatic session detected, which decided its commands.
    std::optional<GPSType> detected;
    /// The records the session reproduced, and what they hold.
    int records = 0;
    qint64 receivedBytes = 0;
    qint64 sentBytes = 0;
    int baudRateChanges = 0;
    /// How long the replayed session lasted on its clock.
    std::chrono::microseconds duration{0};

    /// The driver reproduced every record of the recording.
    [[nodiscard]] bool passed() const { return error.isEmpty() && !divergence; }

    /// The divergence or the error; empty when passed().
    [[nodiscard]] QString failure() const { return divergence ? divergence->text() : error; }
};

/// Runs GPSDriver, as the receiver worker does, against the recording at @a recording replayed as a dialogue
/// (GPSDialogueTransport), on a virtual clock: the session the recording's settings describe. The driver configures
/// the receiver and then receives in the worker's slices until the recording ends. The worker's idle limit is not
/// applied: the replay reads on while records remain.
/// @a interrupted is asked between receives and ends the replay with an error.
[[nodiscard]] GPSDialogueResult replayDialogue(const QString& recording, const std::function<bool()>& interrupted = {});

}  // namespace GPSTest
