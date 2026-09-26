#pragma once

#include <array>
#include <chrono>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <variant>

#include <QtCore/QByteArrayView>
#include <QtCore/QString>

#include "GPSCommandSequence.h"
#include "GPSDeadline.h"
#include "GPSProtocolError.h"
#include "GPSProtocolEvent.h"
#include "GPSReceiverFamily.h"
#include "GPSRuntimeIO.h"
#include "GPSTask.h"

class GPSCommandChannel;
class GPSDecodeContext;
class GPSProtocolRuntime;
class GPSRawAckMatcher;
class GPSStreamDemux;

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
};

/// Probes the receiver at the rate the link was just set to.
using GPSBaudProbeFunction = std::function<GPSTask<GPSBaudProbe>(unsigned baud)>;

/// Tightens the operation deadline for its lifetime: every read, write, wait and command it encloses completes by
/// the earlier of the enclosing deadline and now + timeout. Scopes nest, and destruction restores the enclosing one.
class [[nodiscard]] GPSDeadlineScope
{
public:
    GPSDeadlineScope(GPSCommandChannel& channel, std::chrono::milliseconds timeout);
    ~GPSDeadlineScope();

    GPSDeadlineScope(const GPSDeadlineScope&) = delete;
    GPSDeadlineScope& operator=(const GPSDeadlineScope&) = delete;

    /// Tightens this scope's deadline further to @a untilUs.
    void limitUntil(uint64_t untilUs);

private:
    GPSCommandChannel& _channel;
    GPSDeadline _previous;
};

/// The only I/O surface of a configurator or streaming service. Operations are awaitables the runtime resumes after
/// performing the blocking I/O they request; none block themselves. After a sticky failure (a transport fault,
/// cancellation or protocol failure), every operation completes immediately with failure, so protocol code needs no
/// failure checks between steps. Each command attempt records evidence: its label, outcome, start and finish, and the
/// accepted, written and uncertain byte counts of the writes it made.
class GPSCommandChannel
{
public:
    /// Bytes requested by each receive or reply read.
    static constexpr size_t READ_CHUNK_SIZE = 150;
    /// Budget of the streaming services that run after each receive.
    static constexpr std::chrono::milliseconds SERVICE_TIMEOUT{5000};

    class [[nodiscard]] ReadAwaitable
    {
    public:
        bool await_ready() { return _channel._prepareRead(*this); }

        void await_suspend(std::coroutine_handle<> handle) { _channel._post(_request, handle); }

        /// @return bytes read, 0 when nothing arrived, or -1 after a sticky failure.
        int await_resume() { return _ready ? _immediate : _channel._finishRead(_request); }

    private:
        friend GPSCommandChannel;

        ReadAwaitable(GPSCommandChannel& channel, std::span<uint8_t> buffer, std::chrono::milliseconds timeout)
            : _channel(channel)
            , _timeout(timeout)
        {
            _request.kind = GPSIORequest::Kind::Read;
            _request.readBuffer = buffer;
        }

        GPSCommandChannel& _channel;
        GPSIORequest _request;
        std::chrono::milliseconds _timeout;
        int _immediate = 0;
        bool _ready = false;
    };

    class [[nodiscard]] ReadAndDecodeAwaitable
    {
    public:
        bool await_ready() { return _read.await_ready(); }

        void await_suspend(std::coroutine_handle<> handle) { _read.await_suspend(handle); }

        GPSReceiveUpdates await_resume() { return _channel._decodeRead(_buffer, _read.await_resume()); }

    private:
        friend GPSCommandChannel;

        ReadAndDecodeAwaitable(GPSCommandChannel& channel, std::chrono::milliseconds timeout)
            : _channel(channel)
            , _read(channel, _buffer, timeout)
        {}

        std::array<uint8_t, READ_CHUNK_SIZE> _buffer{};
        GPSCommandChannel& _channel;
        ReadAwaitable _read;
    };

    class [[nodiscard]] WriteAwaitable
    {
    public:
        bool await_ready() { return _channel._prepareWrite(*this); }

        void await_suspend(std::coroutine_handle<> handle) { _channel._post(_request, handle); }

        /// @return true when every byte was accepted and written.
        bool await_resume() { return !_ready && _channel._finishWrite(_request); }

    private:
        friend GPSCommandChannel;

        WriteAwaitable(GPSCommandChannel& channel, QByteArrayView bytes)
            : _channel(channel)
            , _bytes(bytes)
        {
            _request.kind = GPSIORequest::Kind::Write;
        }

        GPSCommandChannel& _channel;
        GPSIORequest _request;
        QByteArrayView _bytes;
        bool _ready = false;
    };

    class [[nodiscard]] BaudrateAwaitable
    {
    public:
        bool await_ready() { return _channel._prepareBaudrate(*this); }

        void await_suspend(std::coroutine_handle<> handle) { _channel._post(_request, handle); }

        /// @return true when the link runs at the requested rate.
        bool await_resume() { return !_ready && _channel._finishBaudrate(_request); }

    private:
        friend GPSCommandChannel;

        BaudrateAwaitable(GPSCommandChannel& channel, unsigned baudrate)
            : _channel(channel)
        {
            _request.kind = GPSIORequest::Kind::SetBaudrate;
            _request.baudrate = baudrate;
        }

        GPSCommandChannel& _channel;
        GPSIORequest _request;
        bool _ready = false;
    };

    class [[nodiscard]] WaitAwaitable
    {
    public:
        bool await_ready() { return _channel._prepareWait(*this); }

        void await_suspend(std::coroutine_handle<> handle) { _channel._post(_request, handle); }

        void await_resume()
        {
            if (!_ready) {
                _channel._finishWait(_request);
            }
        }

    private:
        friend GPSCommandChannel;

        WaitAwaitable(GPSCommandChannel& channel, std::chrono::microseconds duration)
            : _channel(channel)
        {
            _request.kind = GPSIORequest::Kind::Wait;
            _request.duration = duration;
        }

        GPSCommandChannel& _channel;
        GPSIORequest _request;
        bool _ready = false;
    };

    GPSCommandChannel(const GPSCommandChannel&) = delete;
    GPSCommandChannel& operator=(const GPSCommandChannel&) = delete;

    // Clock and deadlines

    [[nodiscard]] uint64_t nowUs() const;

    [[nodiscard]] std::chrono::milliseconds remainingUntil(uint64_t deadlineUs) const
    {
        return GPSDeadline{deadlineUs}.remaining(nowUs());
    }

    [[nodiscard]] GPSDeadline operationDeadline() const { return _operationDeadline; }

    /// Deadline of the current command attempt, set by beginCommand().
    [[nodiscard]] GPSDeadline commandDeadline() const { return _commandDeadline; }

    [[nodiscard]] GPSDeadlineScope deadlineScope(std::chrono::milliseconds timeout) { return {*this, timeout}; }

    // Sticky failure

    [[nodiscard]] bool failed() const { return _error != GPSProtocolError::None; }

    [[nodiscard]] GPSProtocolError error() const { return _error; }

    [[nodiscard]] const QString& errorDetail() const { return _errorDetail; }

    void setErrorDetail(const QString& detail) { _errorDetail = detail; }

    /// Records @a failure, Protocol or ConsentRequired, unless a failure is already recorded; clears the detail it
    /// replaces.
    void failControl(GPSProtocolError failure = GPSProtocolError::Protocol);

    /// The command outcome that reports the sticky failure; Pending while healthy.
    [[nodiscard]] GPSCommandOutcome failureOutcome() const;

    // Command evidence

    /// Starts a command attempt: retires a pending one as Written, stamps the start and sets its deadline.
    void beginCommand(GPSConfigurationStep step);

    /// Retires the current attempt and reports it; repeated completion returns the retained evidence unchanged.
    GPSCommandResult completeCommand(GPSCommandOutcome outcome);

    void finishCommand(GPSCommandOutcome outcome) { (void) completeCommand(outcome); }

    [[nodiscard]] const GPSCommandResult& currentCommand() const { return _command; }

    /// Retires a command still pending when configuration ends, as Written.
    void finishConfigurationEvidence() { finishCommand(GPSCommandOutcome::Written); }

    // Blocking operations

    /// Reads raw bytes without decoding them.
    ReadAwaitable read(std::span<uint8_t> buffer, std::chrono::milliseconds timeout)
    {
        return {*this, buffer, timeout};
    }

    /// Reads one bounded chunk and decodes it. @return the chunk's updates.
    ReadAndDecodeAwaitable readAndDecode(std::chrono::milliseconds timeout) { return {*this, timeout}; }

    /// Writes all of @a bytes under the current command deadline. A null view is an invalid argument. An unsupported
    /// write fails the current command without a sticky failure.
    WriteAwaitable write(QByteArrayView bytes) { return {*this, bytes}; }

    /// An unsupported change fails without a sticky failure.
    BaudrateAwaitable setBaudrate(unsigned baudrate) { return {*this, baudrate}; }

    /// Waits for @a duration, capped by the operation deadline.
    WaitAwaitable wait(std::chrono::microseconds duration) { return {*this, duration}; }

    // Commands

    /// Starts one command attempt under its own deadline scope and writes it; its reply is awaited separately.
    GPSTask<bool> writeCommand(GPSConfigurationStep step, QByteArrayView bytes);

    /// Decodes received traffic until @a reply resolves the current command, its deadline expires, or I/O fails.
    GPSTask<GPSCommandResult> awaitReply(GPSReplyPoll reply);

    /// Writes @a bytes and waits until a decoder resolves the reply through GPSDecodeContext, the step times out, or
    /// I/O fails. Received traffic keeps decoding while the reply is pending.
    GPSTask<GPSCommandResult> transact(GPSConfigurationStep step, QByteArrayView bytes)
    {
        return _transact(std::move(step), bytes, std::monostate{});
    }

    /// As above, also resolved from the raw received bytes.
    GPSTask<GPSCommandResult> transact(GPSConfigurationStep step, QByteArrayView bytes, GPSRawAckMatcher& reply)
    {
        return _transact(std::move(step), bytes, &reply);
    }

    /// As above, also resolved by @a reply from each text reply a decoder offers.
    GPSTask<GPSCommandResult> transact(GPSConfigurationStep step, QByteArrayView bytes, GPSTextMatcher reply)
    {
        return _transact(std::move(step), bytes, std::move(reply));
    }

    /// As above, also resolved by @a reply from each dispatched frame.
    GPSTask<GPSCommandResult> transact(GPSConfigurationStep step, QByteArrayView bytes, GPSFrameMatcher reply)
    {
        return _transact(std::move(step), bytes, std::move(reply));
    }

    /// Runs @a sequence in order until a required step fails or I/O fails; failed attempts retry up to their count.
    GPSTask<GPSCommandSequence::Result> runSequence(const GPSCommandSequence& sequence);

    /// Tries each of @a candidates, or only @a baud when it is nonzero, until @a probe finds the receiver at the rate
    /// passed to it. A link failure, a Stop result, or a sticky failure ends the search. Without @a baud, the rate
    /// receiver detection found (GPSConfig::detectedBaud) is tried first, even when it is not a candidate.
    GPSTask<GPSBaudDetection> detectBaud(std::span<const unsigned> candidates, unsigned baud,
                                         GPSBaudProbeFunction probe);

    // Receiving

    /// One receive cycle without streaming services: reads under @a timeout in the family's read slices until the
    /// family reports the cycle complete, the deadline passes, or I/O fails. @return the accumulated updates.
    GPSTask<GPSReceiveUpdates> receiveCycle(std::chrono::milliseconds timeout);

    /// The family's full streaming receive, as GPSProtocolRuntime::receive() runs it.
    GPSTask<GPSReceiveUpdates> receive(std::chrono::milliseconds timeout);

    /// Repeats the full streaming receive, each with @a duration, until @a duration has passed or I/O fails.
    GPSTask<void> receiveFor(std::chrono::milliseconds duration);

    /// Repeats receive cycles of @a duration until @a done, @a duration has passed, or I/O fails.
    /// @return whether @a done holds; false after a sticky failure.
    GPSTask<bool> receiveUntil(std::function<bool()> done, std::chrono::milliseconds duration);

    /// Runs the family's streaming services once under SERVICE_TIMEOUT, then flushes. Nested calls do nothing.
    GPSTask<void> serviceControls();

    // Decoding without I/O

    /// Flushes decoder state and delivers the resulting batch. @return its updates.
    GPSReceiveUpdates flush();

    [[nodiscard]] GPSStreamDemux& stream();

    /// The decoding context, for publishing events from configuration or streaming services. Events published
    /// between decodes join the next decoded batch.
    [[nodiscard]] GPSDecodeContext& context();

    /// Validates @a config against the family's configuration support, logging a rejected request.
    [[nodiscard]] bool validateConfiguration(const GPSConfig& config) const;

    [[nodiscard]] GPSLogCategory logCategory() const;

    // Reply state, shared with GPSDecodeContext

    /// True while a transact() reply is outstanding and unresolved.
    [[nodiscard]] bool replyPending() const { return _reply == GPSCommandOutcome::Pending; }

    /// Resolves the outstanding transact() reply; Pending and later replies are ignored.
    void resolveReply(GPSCommandOutcome outcome)
    {
        if (replyPending()) {
            _reply = outcome;
        }
    }

private:
    friend class GPSDeadlineScope;
    friend class GPSDecodeContext;
    friend class GPSProtocolRuntime;

    using ReplyMatcher = std::variant<std::monostate, GPSRawAckMatcher*, GPSTextMatcher, GPSFrameMatcher>;

    explicit GPSCommandChannel(GPSProtocolRuntime& runtime);

    GPSTask<GPSCommandResult> _transact(GPSConfigurationStep step, QByteArrayView bytes, ReplyMatcher matcher);
    void _clearReply();
    void _resetError();
    void _offerText(std::string_view reply);
    void _offerFrame(const GPSFrame& frame);
    void _offerRaw(std::span<const uint8_t> bytes);

    bool _prepareRead(ReadAwaitable& awaitable);
    bool _prepareWrite(WriteAwaitable& awaitable);
    bool _prepareBaudrate(BaudrateAwaitable& awaitable);
    bool _prepareWait(WaitAwaitable& awaitable);
    void _post(GPSIORequest& request, std::coroutine_handle<> continuation);
    int _finishRead(const GPSIORequest& request);
    GPSReceiveUpdates _decodeRead(std::span<const uint8_t> buffer, int count);
    bool _finishWrite(const GPSIORequest& request);
    bool _finishBaudrate(const GPSIORequest& request);
    void _finishWait(const GPSIORequest& request);

    GPSProtocolRuntime& _runtime;
    GPSProtocolError _error = GPSProtocolError::None;
    QString _errorDetail;
    GPSDeadline _operationDeadline;
    bool _commandCompleted = true;
    GPSCommandResult _command;
    GPSDeadline _commandDeadline;
    std::optional<GPSCommandOutcome> _reply;
    GPSRawAckMatcher* _rawReply = nullptr;
    GPSTextMatcher _textReply;
    GPSFrameMatcher _frameReply;
    unsigned _detectedBaud = 0;
    bool _servicingControls = false;
};
