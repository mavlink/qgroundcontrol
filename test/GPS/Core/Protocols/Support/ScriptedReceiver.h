#pragma once

#include <chrono>
#include <deque>
#include <functional>
#include <optional>
#include <span>

#include <QtCore/QByteArray>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QList>

#include "GPSCancellation.h"
#include "GPSDeadline.h"
#include "GPSTransport.h"
#include "Protocols/Support/GPSTestClock.h"

struct GPSRuntimeIO;

namespace GPSTest {

class ScriptedReceiver : public GPSTransport
{
public:
    struct WriteContext
    {
        QDeadlineTimer transportDeadline;
        GPSDeadline protocolDeadline;
        bool hasProtocolDeadline = false;
    };

    class Model
    {
    public:
        virtual ~Model();

        virtual void reset(ScriptedReceiver& receiver);
        virtual std::optional<QByteArray> takeCommand(QByteArray& pending);
        virtual GPSWriteResult handleCommand(ScriptedReceiver& receiver, const QByteArray& command,
                                             const WriteContext& context);
        virtual std::optional<bool> handleBaudrate(ScriptedReceiver& receiver, unsigned baudrate);
        virtual void onTransportReadWait(ScriptedReceiver& receiver, std::chrono::milliseconds timeout);
        virtual void onProtocolReadWait(ScriptedReceiver& receiver, GPSDeadline deadline);
        virtual int readChunkSize(const ScriptedReceiver& receiver, int requested, int available) const;
        virtual bool coalesceReads(const ScriptedReceiver& receiver) const;

    protected:
        /// The next CR/LF-terminated command of @a pending, terminator included, for a receiver of command lines.
        static std::optional<QByteArray> takeLine(QByteArray& pending);
    };

    explicit ScriptedReceiver(GPSCancelToken cancelToken, Model* model = nullptr);
    /// cancel() cancels through cancelSource.
    explicit ScriptedReceiver(GPSCancelSource cancelSource, Model* model = nullptr);
    ~ScriptedReceiver() override;

    GPSOpenResult open() override;

    bool fatalError() const override { return _fatalError; }

    unsigned fixedBaudrate() const override { return _fixedBaudrate; }

    std::chrono::milliseconds configurationWriteTimeout() const override;

    bool setBaudrate(unsigned baudrate) override;

    GPSReadResult read(std::span<uint8_t> buffer, std::chrono::milliseconds timeout) override;

    /// Runtime services over this receiver: read, write and setBaudrate replace those of @a io, which keeps its clock.
    GPSRuntimeIO makeIO(GPSRuntimeIO io);

    void setModel(Model* model);

    /// Transport reads wait on @a clock as protocol reads do: the model acts until the read's deadline, and a read
    /// that finds no data moves the clock to it. Without a clock, transport reads never wait.
    void setClock(GPSTestClock* clock) { _clock = clock; }

    void setFatalError(bool fatalError) { _fatalError = fatalError; }

    void setFixedBaudrate(unsigned baudrate) { _fixedBaudrate = baudrate; }

    void setReceiverBaudrate(unsigned baudrate) { _receiverBaudrate = baudrate; }

    void setBaudrateEnforced(bool enforced) { _baudrateEnforced = enforced; }

    void setOpenHandler(std::function<std::optional<GPSOpenResult>()> handler) { _openHandler = std::move(handler); }

    void setReadHandler(std::function<std::optional<GPSReadResult>(uint8_t*, int, std::chrono::milliseconds)> handler)
    {
        _readHandler = std::move(handler);
    }

    void setWriteHandler(std::function<std::optional<GPSWriteResult>(const QByteArray&, const WriteContext&)> handler)
    {
        _writeHandler = std::move(handler);
    }

    void setConfigurationWriteTimeoutHandler(std::function<std::chrono::milliseconds()> handler)
    {
        _configurationWriteTimeoutHandler = std::move(handler);
    }

    void setBaudrateHandler(std::function<std::optional<bool>(unsigned)> handler)
    {
        _baudrateHandler = std::move(handler);
    }

    void queueReply(const QByteArray& bytes);
    void clearReplies();

    bool hasQueuedReadData() const { return !_readSteps.empty(); }

    void failNextRead(GPSReadResult result) { _nextReadResult = std::move(result); }

    void cancel();

    unsigned hostBaudrate() const { return _hostBaudrate; }

    unsigned receiverBaudrate() const { return _receiverBaudrate; }

    const QList<QByteArray>& commands() const { return _commands; }

    void clearCommands() { _commands.clear(); }

    /// Operations the host performed: opens, reads, write calls and baud rate changes, on either path.
    int opens() const { return _opens; }

    int reads() const { return _reads; }

    int writes() const { return _writes; }

    int baudChanges() const { return _baudChanges; }

protected:
    GPSWriteResult writeData(QByteArrayView bytes, QDeadlineTimer deadline) override;

private:
    GPSReadResult _read(std::span<uint8_t> buffer, std::chrono::milliseconds timeout,
                        std::optional<GPSDeadline> deadline);
    GPSWriteResult _write(const QByteArray& bytes, const WriteContext& context);
    bool _setBaudrate(unsigned baudrate);
    GPSReadResult _readOne(std::span<uint8_t> buffer);
    bool _baudrateMatches() const;
    void _attachModel(Model* model);

    struct ReadStep
    {
        QByteArray bytes;
        qsizetype offset = 0;
    };

    Model* _model = nullptr;
    GPSTestClock* _clock = nullptr;
    GPSCancelSource _cancelSource;
    std::deque<ReadStep> _readSteps;
    QList<QByteArray> _commands;
    QByteArray _pendingWrites;
    std::optional<GPSReadResult> _nextReadResult;
    std::function<std::optional<GPSOpenResult>()> _openHandler;
    std::function<std::optional<GPSReadResult>(uint8_t*, int, std::chrono::milliseconds)> _readHandler;
    std::function<std::optional<GPSWriteResult>(const QByteArray&, const WriteContext&)> _writeHandler;
    std::function<std::chrono::milliseconds()> _configurationWriteTimeoutHandler;
    std::function<std::optional<bool>(unsigned)> _baudrateHandler;
    unsigned _hostBaudrate = 0;
    unsigned _receiverBaudrate = 0;
    unsigned _fixedBaudrate = 0;
    bool _baudrateEnforced = false;
    bool _fatalError = false;
    int _opens = 0;
    int _reads = 0;
    int _writes = 0;
    int _baudChanges = 0;
};

/// Decorates another model: every call reaches it unless the decorator overrides that call.
class ForwardingModel : public ScriptedReceiver::Model
{
public:
    explicit ForwardingModel(ScriptedReceiver::Model& model)
        : _model(model)
    {}

    void reset(ScriptedReceiver& receiver) override { _model.reset(receiver); }

    std::optional<QByteArray> takeCommand(QByteArray& pending) override { return _model.takeCommand(pending); }

    GPSWriteResult handleCommand(ScriptedReceiver& receiver, const QByteArray& command,
                                 const ScriptedReceiver::WriteContext& context) override
    {
        return _model.handleCommand(receiver, command, context);
    }

    std::optional<bool> handleBaudrate(ScriptedReceiver& receiver, unsigned baudrate) override
    {
        return _model.handleBaudrate(receiver, baudrate);
    }

    void onTransportReadWait(ScriptedReceiver& receiver, std::chrono::milliseconds timeout) override
    {
        _model.onTransportReadWait(receiver, timeout);
    }

    void onProtocolReadWait(ScriptedReceiver& receiver, GPSDeadline deadline) override
    {
        _model.onProtocolReadWait(receiver, deadline);
    }

    int readChunkSize(const ScriptedReceiver& receiver, int requested, int available) const override
    {
        return _model.readChunkSize(receiver, requested, available);
    }

    bool coalesceReads(const ScriptedReceiver& receiver) const override { return _model.coalesceReads(receiver); }

protected:
    ScriptedReceiver::Model& _model;
};

}  // namespace GPSTest
