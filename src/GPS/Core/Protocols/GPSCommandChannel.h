#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <variant>
#include <vector>

#include <QtCore/QByteArrayView>
#include <QtCore/QString>

#include "GPSCommand.h"
#include "GPSDeadline.h"
#include "GPSFamilyProtocol.h"
#include "GPSProtocolEvent.h"

class GPSCommandChannel;
class GPSDecodeContext;
class GPSProtocolRuntime;
class GPSStreamDemux;

/// How the link answered a baud rate change.
enum class [[nodiscard]] GPSBaudStatus
{
    Configured,
    Unsupported,
    Cancelled,
    Error
};

enum class GPSBaudProbe
{
    Found,
    TryNext,
    /// The receiver answered but is not usable, so no other rate is tried.
    Stop,
};

struct GPSBaudDetection
{
    bool found = false;
    /// The rate that was found, or the last rate attempted.
    unsigned baud = 0;
    /// The link could not be set to the last rate attempted.
    bool linkFailed = false;
    /// The rates the link ran at while probing, in order.
    std::vector<unsigned> probed{};
};

/// @a rates as a diagnostic names them, such as "38400, 9600".
[[nodiscard]] QString gpsRateList(std::span<const unsigned> rates);

/// Probes the receiver at the rate the link was just set to.
using GPSBaudProbeFunction = std::function<GPSBaudProbe()>;

/// Tightens the operation deadline for its lifetime: every read, write, wait and command it encloses completes by
/// the earlier of the enclosing deadline and the scope's own. Scopes nest, and destruction restores the enclosing one.
class [[nodiscard]] GPSDeadlineScope
{
public:
    GPSDeadlineScope(GPSCommandChannel& channel, std::chrono::milliseconds timeout);
    GPSDeadlineScope(GPSCommandChannel& channel, GPSDeadline deadline);
    ~GPSDeadlineScope();

    Q_DISABLE_COPY(GPSDeadlineScope)

private:
    void _limitUntil(uint64_t untilUs);

    GPSCommandChannel& _channel;
    GPSDeadline _previous;
};

/// The only I/O surface of a configurator or streaming service. Operations block on the caller thread; the runtime
/// performs their I/O through its GPSRuntimeIO. After a sticky failure (a transport fault, cancellation or protocol
/// failure), every operation returns failure immediately without I/O, so protocol code needs no failure checks between
/// steps. Each command attempt records evidence: its label, outcome, start and finish, and the accepted, written and
/// uncertain byte counts of the writes it made.
class GPSCommandChannel
{
public:
    /// Bytes requested by each receive or reply read.
    static constexpr size_t READ_CHUNK_SIZE = 150;
    /// Budget of the streaming services that run after each receive.
    static constexpr std::chrono::milliseconds SERVICE_TIMEOUT{5000};

    Q_DISABLE_COPY(GPSCommandChannel)

    // Clock and deadlines

    [[nodiscard]] uint64_t nowUs() const;

    [[nodiscard]] GPSDeadline operationDeadline() const { return _operationDeadline; }

    /// Deadline of the current command attempt, set by beginCommand().
    [[nodiscard]] GPSDeadline commandDeadline() const { return _commandDeadline; }

    [[nodiscard]] GPSDeadlineScope deadlineScope(std::chrono::milliseconds timeout) { return {*this, timeout}; }

    [[nodiscard]] GPSDeadlineScope deadlineScope(GPSDeadline deadline) { return {*this, deadline}; }

    // Sticky failure

    [[nodiscard]] bool failed() const { return _error != GPSProtocolError::None; }

    [[nodiscard]] GPSProtocolError error() const { return _error; }

    [[nodiscard]] const QString& errorDetail() const { return _errorDetail; }

    void setErrorDetail(const QString& detail) { _errorDetail = detail; }

    /// Records @a failure, Protocol or ConsentRequired, unless a failure is already recorded; clears the detail it
    /// replaces.
    void failControl(GPSProtocolError failure = GPSProtocolError::Protocol);

    /// Records a protocol failure described by @a detail unless a failure is already recorded, whose detail stays.
    void failControl(const QString& detail);

    /// Fails control over the command that ended @a result, naming it and the family's receiver in the detail, unless a
    /// transport failure or cancellation already failed the attempt.
    void failSequence(const GPSCommandSequence::Result& result);

    /// Fails control as no receiver of the family answering at @a rates, or as the link refusing the requested rate
    /// when none was probed, unless a failure is already recorded.
    void failNoAnswer(std::span<const unsigned> rates);

    /// The command outcome that reports the sticky failure; Pending while healthy.
    [[nodiscard]] GPSCommandOutcome failureOutcome() const;

    // Command evidence

    /// Starts a command attempt: retires a pending one as Written, stamps the start and sets its deadline. After a
    /// sticky failure no attempt starts: a completed attempt is replaced by this one, already ended by the failure.
    void beginCommand(GPSConfigurationStep step);

    /// Retires the current attempt and reports it; repeated completion returns the retained evidence unchanged.
    GPSConfigurationEvidence completeCommand(GPSCommandOutcome outcome);

    void finishCommand(GPSCommandOutcome outcome) { (void) completeCommand(outcome); }

    [[nodiscard]] const GPSConfigurationEvidence& currentCommand() const { return _command; }

    // Blocking operations

    /// Reads raw bytes without decoding them. @return bytes read, 0 when nothing arrived, or -1 after a sticky failure.
    int read(std::span<uint8_t> buffer, std::chrono::milliseconds timeout);

    /// Writes all of @a bytes under the current command deadline. A null view is an invalid argument. A command that
    /// is late before any of its bytes were written, whether found here or by the transport, or an unsupported write,
    /// fails the current command without a sticky failure. @return true when every byte was accepted and written.
    bool write(QByteArrayView bytes);

    /// An unsupported change fails without a sticky failure; a failed one names the rate in the error detail.
    /// @return true when the link runs at the requested rate.
    bool setBaudrate(unsigned baudrate);

    /// Waits for @a duration, capped by the operation deadline.
    void wait(std::chrono::microseconds duration);

    // Commands

    /// Starts one command attempt under its own deadline scope and writes it; awaitReply() waits for its reply.
    bool writeCommand(GPSConfigurationStep step, QByteArrayView bytes);

    /// Decodes received traffic until @a reply resolves the current command, its deadline expires, or I/O fails.
    GPSConfigurationEvidence awaitReply(GPSReplyPoll reply);

    /// Writes @a bytes and waits until a decoder resolves the reply through GPSDecodeContext, the step times out, or
    /// I/O fails. Received traffic keeps decoding while the reply is pending.
    GPSConfigurationEvidence transact(GPSConfigurationStep step, QByteArrayView bytes)
    {
        return _transact(std::move(step), bytes, std::monostate{});
    }

    /// As above, also resolved by @a reply from each text reply a decoder offers.
    GPSConfigurationEvidence transact(GPSConfigurationStep step, QByteArrayView bytes, GPSTextMatcher reply)
    {
        return _transact(std::move(step), bytes, std::move(reply));
    }

    /// As above, also resolved by @a reply from each dispatched frame.
    GPSConfigurationEvidence transact(GPSConfigurationStep step, QByteArrayView bytes, GPSFrameMatcher reply)
    {
        return _transact(std::move(step), bytes, std::move(reply));
    }

    /// As above, for one command of a sequence: its step, bytes and text reply, or its raw reply matched in the
    /// received bytes.
    GPSConfigurationEvidence transact(const GPSCommandSequence::Command& command);

    /// Runs @a sequence in order until a required step fails or I/O fails; failed attempts retry up to their count.
    GPSCommandSequence::Result runSequence(const GPSCommandSequence& sequence);

    /// Runs @a sequence and fails control over a failed step as failSequence() does. @return whether it succeeded.
    [[nodiscard]] bool runRequired(const GPSCommandSequence& sequence);

    /// Tries each of @a candidates, or only @a baud when it is nonzero, until @a probe finds the receiver at the rate
    /// the link was set to. A candidate the link cannot run at is skipped; a link failure, a Stop result, or a sticky
    /// failure ends the search. Without @a baud, the rate receiver detection found (GPSConfig::detectedBaud) is tried
    /// first, even when it is not a candidate.
    GPSBaudDetection detectBaud(std::span<const unsigned> candidates, unsigned baud, GPSBaudProbeFunction probe);

    // Receiving

    /// One receive cycle without streaming services: reads under @a timeout in the family's read slices until the
    /// family reports the cycle complete, the deadline passes, or I/O fails. @return the accumulated updates.
    GPSReceiveUpdates receiveCycle(std::chrono::milliseconds timeout);

    /// Repeats the family's full streaming receive until @a duration has passed or I/O fails.
    void receiveFor(std::chrono::milliseconds duration);

    /// Repeats receive cycles until @a done, @a duration has passed, or I/O fails.
    /// @return whether @a done holds; false after a sticky failure.
    bool receiveUntil(std::function<bool()> done, std::chrono::milliseconds duration);

    /// Runs the family's streaming services once under SERVICE_TIMEOUT, then flushes. Nested calls do nothing.
    void serviceControls();

    // Decoding without I/O

    /// Flushes decoder state and delivers the resulting batch. @return its updates.
    GPSReceiveUpdates flush();

    [[nodiscard]] GPSStreamDemux& stream();

    /// The decoding context, for publishing events from configuration or streaming services. Events published
    /// between decodes join the next decoded batch.
    [[nodiscard]] GPSDecodeContext& context();

    [[nodiscard]] GPSLogCategory logCategory() const;

private:
    friend class GPSDeadlineScope;
    friend class GPSDecodeContext;
    friend class GPSProtocolRuntime;

    using ReplyMatcher = std::variant<std::monostate, GPSRawAckMatcher*, GPSTextMatcher, GPSFrameMatcher>;

    explicit GPSCommandChannel(GPSProtocolRuntime& runtime);

    /// True while a transact() reply is outstanding and unresolved.
    [[nodiscard]] bool _replyPending() const { return _reply == GPSCommandOutcome::Pending; }

    /// Resolves the outstanding transact() reply; Pending and later replies are ignored.
    void _resolveReply(GPSCommandOutcome outcome)
    {
        if (_replyPending()) {
            _reply = outcome;
        }
    }

    /// The attempt @a step would be after a sticky failure: never started, ended by the failure.
    [[nodiscard]] GPSConfigurationEvidence _failedAttempt(GPSConfigurationStep step) const;

    /// setBaudrate() with the link's answer; an unsupported rate leaves no failure.
    GPSBaudStatus _setBaudrate(unsigned baudrate);

    /// Reads one bounded chunk and decodes it. @return the chunk's updates.
    GPSReceiveUpdates _readAndDecode(std::chrono::milliseconds timeout);

    [[nodiscard]] QString _commandFailure(const QByteArray& label, GPSCommandOutcome outcome) const;
    /// Names the latest command, if it failed, as the failure's detail when the failure has none.
    void _describeFailure();
    GPSConfigurationEvidence _transact(GPSConfigurationStep step, QByteArrayView bytes, ReplyMatcher matcher);
    void _clearReply();
    void _resetError();
    void _offerText(std::string_view reply);
    void _offerFrame(const GPSFrame& frame);
    void _offerRaw(std::span<const uint8_t> bytes);

    GPSProtocolRuntime& _runtime;
    GPSProtocolError _error = GPSProtocolError::None;
    QString _errorDetail;
    GPSDeadline _operationDeadline;
    bool _commandCompleted = true;
    GPSConfigurationEvidence _command;
    GPSDeadline _commandDeadline;
    std::optional<GPSCommandOutcome> _reply;
    ReplyMatcher _replyMatcher;
    unsigned _detectedBaud = 0;
    bool _servicingControls = false;
};
