#pragma once

#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <optional>
#include <span>

#include <QtCore/QByteArray>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QList>

#include "GPSTransport.h"
#include "Protocols/Contracts/GPSProtocolIO.h"

class ScriptedReceiver : public GPSTransport
{
public:
    struct WriteContext
    {
        QDeadlineTimer transportDeadline;
        GPSDeadline protocolDeadline;
        bool hasTransportDeadline = false;
        bool hasProtocolDeadline = false;
    };

    struct ReadOptions
    {
        ReadOptions() = default;

        int maxChunkSize = 0;
        int delayMs = 0;
        bool drop = false;
        bool garble = false;
        bool coalesce = false;
        std::function<void()> onConsumed;
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
        virtual void onTransportReadWait(ScriptedReceiver& receiver, int timeoutMs);
        virtual void onProtocolReadWait(ScriptedReceiver& receiver, GPSDeadline deadline);
        virtual int readChunkSize(const ScriptedReceiver& receiver, int requested, int available) const;
        virtual bool coalesceReads(const ScriptedReceiver& receiver) const;
    };

    explicit ScriptedReceiver(const std::atomic_bool& requestStop, Model* model = nullptr);
    ScriptedReceiver(std::atomic_bool& requestStop, Model& model);
    ~ScriptedReceiver() override;

    GPSOpenResult open() override;

    bool fatalError() const override { return _fatalError; }

    unsigned fixedBaudrate() const override { return _fixedBaudrate; }

    std::chrono::milliseconds configurationWriteTimeout() const override;

    bool setBaudrate(unsigned baudrate) override;

    GPSReadResult read(uint8_t* buffer, int length, int timeoutMs) override;

    GPSProtocolIO makeIO(GPSProtocolIO io);

    void setModel(Model* model);

    void setFatalError(bool fatalError) { _fatalError = fatalError; }

    void setFixedBaudrate(unsigned baudrate) { _fixedBaudrate = baudrate; }

    void setReceiverBaudrate(unsigned baudrate) { _receiverBaudrate = baudrate; }

    void setBaudrateEnforced(bool enforced) { _baudrateEnforced = enforced; }

    void setOpenResult(std::optional<GPSOpenResult> result) { _openResult = std::move(result); }

    void setBaudrateResult(std::optional<bool> result) { _baudrateResult = result; }

    void setOpenHandler(std::function<std::optional<GPSOpenResult>()> handler) { _openHandler = std::move(handler); }

    void setReadHandler(std::function<std::optional<GPSReadResult>(uint8_t*, int, int)> handler)
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
    void queueReply(const QByteArray& bytes, ReadOptions options);
    void clearReplies();

    bool hasQueuedReadData() const { return !_readSteps.empty(); }

    GPSReadResult readQueued(uint8_t* buffer, int length);

    void failNextRead(GPSReadResult result) { _nextReadResult = std::move(result); }

    void failNextWrite(GPSWriteResult result) { _nextWriteResult = std::move(result); }

    void cancel();

    unsigned hostBaudrate() const { return _hostBaudrate; }

    unsigned receiverBaudrate() const { return _receiverBaudrate; }

    bool baudrateMatches() const;

    const QList<QByteArray>& commands() const { return _commands; }

    void clearCommands() { _commands.clear(); }

    QByteArray takePendingWrites();

protected:
    GPSWriteResult writeData(const uint8_t* buffer, int length, QDeadlineTimer deadline) override;

private:
    GPSReadResult _read(uint8_t* buffer, int length, int timeoutMs, std::optional<GPSDeadline> deadline);
    GPSWriteResult _write(const QByteArray& bytes, const WriteContext& context);
    GPSReadResult _readOne(uint8_t* buffer, int length);
    void _attachModel(Model* model);

    struct ReadStep
    {
        QByteArray bytes;
        qsizetype offset = 0;
        ReadOptions options;
    };

    Model* _model = nullptr;
    std::atomic_bool* _mutableStop = nullptr;
    std::deque<ReadStep> _readSteps;
    QList<QByteArray> _commands;
    QByteArray _pendingWrites;
    std::optional<GPSOpenResult> _openResult;
    std::optional<bool> _baudrateResult;
    std::optional<GPSReadResult> _nextReadResult;
    std::optional<GPSWriteResult> _nextWriteResult;
    std::function<std::optional<GPSOpenResult>()> _openHandler;
    std::function<std::optional<GPSReadResult>(uint8_t*, int, int)> _readHandler;
    std::function<std::optional<GPSWriteResult>(const QByteArray&, const WriteContext&)> _writeHandler;
    std::function<std::chrono::milliseconds()> _configurationWriteTimeoutHandler;
    std::function<std::optional<bool>(unsigned)> _baudrateHandler;
    unsigned _hostBaudrate = 0;
    unsigned _receiverBaudrate = 0;
    unsigned _fixedBaudrate = 0;
    bool _baudrateEnforced = false;
    bool _fatalError = false;
};
