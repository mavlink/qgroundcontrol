#include "Support/ScriptedReceiver.h"

#include <algorithm>
#include <cstring>

#include <QtCore/QThread>

ScriptedReceiver::Model::~Model() = default;

void ScriptedReceiver::Model::reset(ScriptedReceiver& receiver)
{
    Q_UNUSED(receiver)
}

std::optional<QByteArray> ScriptedReceiver::Model::takeCommand(QByteArray& pending)
{
    if (pending.isEmpty()) {
        return std::nullopt;
    }
    const QByteArray command = std::move(pending);
    pending.clear();
    return command;
}

GPSWriteResult ScriptedReceiver::Model::handleCommand(ScriptedReceiver& receiver, const QByteArray& command,
                                                      const WriteContext& context)
{
    Q_UNUSED(receiver)
    Q_UNUSED(context)
    const int length = command.size();
    return {GPSWriteStatus::Completed, length, length};
}

std::optional<bool> ScriptedReceiver::Model::handleBaudrate(ScriptedReceiver& receiver, unsigned baudrate)
{
    Q_UNUSED(receiver)
    Q_UNUSED(baudrate)
    return std::nullopt;
}

void ScriptedReceiver::Model::onTransportReadWait(ScriptedReceiver& receiver, int timeoutMs)
{
    Q_UNUSED(receiver)
    Q_UNUSED(timeoutMs)
}

void ScriptedReceiver::Model::onProtocolReadWait(ScriptedReceiver& receiver, GPSDeadline deadline)
{
    Q_UNUSED(receiver)
    Q_UNUSED(deadline)
}

int ScriptedReceiver::Model::readChunkSize(const ScriptedReceiver& receiver, int requested, int available) const
{
    Q_UNUSED(receiver)
    return std::min(requested, available);
}

bool ScriptedReceiver::Model::coalesceReads(const ScriptedReceiver& receiver) const
{
    Q_UNUSED(receiver)
    return false;
}

ScriptedReceiver::ScriptedReceiver(const std::atomic_bool& requestStop, Model* model)
    : GPSTransport(requestStop)
{
    _attachModel(model);
}

ScriptedReceiver::ScriptedReceiver(std::atomic_bool& requestStop, Model& model)
    : GPSTransport(requestStop)
    , _mutableStop(&requestStop)
{
    _attachModel(&model);
}

ScriptedReceiver::~ScriptedReceiver() = default;

GPSOpenResult ScriptedReceiver::open()
{
    clearReplies();
    _pendingWrites.clear();
    if (_model) {
        _model->reset(*this);
    }
    if (_openHandler) {
        if (const auto result = _openHandler()) {
            return *result;
        }
    }
    if (_openResult) {
        return *_openResult;
    }
    return {GPSOpenStatus::Opened};
}

bool ScriptedReceiver::setBaudrate(unsigned baudrate)
{
    _hostBaudrate = baudrate;
    if (_baudrateHandler) {
        if (const auto accepted = _baudrateHandler(baudrate)) {
            return *accepted;
        }
    }
    if (_model) {
        if (const auto accepted = _model->handleBaudrate(*this, baudrate)) {
            return *accepted;
        }
    }
    if (_baudrateResult) {
        return *_baudrateResult;
    }
    return true;
}

std::chrono::milliseconds ScriptedReceiver::configurationWriteTimeout() const
{
    if (_configurationWriteTimeoutHandler) {
        return _configurationWriteTimeoutHandler();
    }
    return GPSTransport::configurationWriteTimeout();
}

GPSReadResult ScriptedReceiver::read(uint8_t* buffer, int length, int timeoutMs)
{
    return _read(buffer, length, timeoutMs, std::nullopt);
}

GPSProtocolIO ScriptedReceiver::makeIO(GPSProtocolIO io)
{
    io.read = [this](std::span<uint8_t> bytes, GPSDeadline deadline) {
        return _read(bytes.data(), static_cast<int>(bytes.size()), 0, deadline);
    };
    io.write = [this](std::span<const uint8_t> bytes, GPSDeadline deadline) {
        WriteContext context;
        context.protocolDeadline = deadline;
        context.hasProtocolDeadline = true;
        return _write(QByteArray(reinterpret_cast<const char*>(bytes.data()), static_cast<qsizetype>(bytes.size())),
                      context);
    };
    io.setBaudrate = [this](unsigned baudrate) {
        _hostBaudrate = baudrate;
        if (_baudrateHandler) {
            if (const auto accepted = _baudrateHandler(baudrate)) {
                return *accepted ? GPSBaudStatus::Configured : GPSBaudStatus::Unsupported;
            }
        }
        if (_model) {
            if (const auto accepted = _model->handleBaudrate(*this, baudrate)) {
                return *accepted ? GPSBaudStatus::Configured : GPSBaudStatus::Unsupported;
            }
        }
        if (_baudrateResult) {
            return *_baudrateResult ? GPSBaudStatus::Configured : GPSBaudStatus::Unsupported;
        }
        return GPSBaudStatus::Configured;
    };
    return io;
}

void ScriptedReceiver::setModel(Model* model)
{
    _attachModel(model);
}

void ScriptedReceiver::queueReply(const QByteArray& bytes)
{
    queueReply(bytes, ReadOptions{});
}

void ScriptedReceiver::queueReply(const QByteArray& bytes, ReadOptions options)
{
    if (bytes.isEmpty() && !options.onConsumed) {
        return;
    }
    QByteArray queued = bytes;
    if (options.garble && !queued.isEmpty()) {
        queued[queued.size() - 1] = static_cast<char>(queued[queued.size() - 1] ^ 0x01);
    }
    _readSteps.push_back(ReadStep{.bytes = std::move(queued), .options = std::move(options)});
}

void ScriptedReceiver::clearReplies()
{
    _readSteps.clear();
}

GPSReadResult ScriptedReceiver::readQueued(uint8_t* buffer, int length)
{
    return _readOne(buffer, length);
}

void ScriptedReceiver::cancel()
{
    if (_mutableStop) {
        _mutableStop->store(true);
    }
}

bool ScriptedReceiver::baudrateMatches() const
{
    return !_baudrateEnforced || !_receiverBaudrate || _hostBaudrate == _receiverBaudrate;
}

QByteArray ScriptedReceiver::takePendingWrites()
{
    QByteArray pending = std::move(_pendingWrites);
    _pendingWrites.clear();
    return pending;
}

GPSWriteResult ScriptedReceiver::writeData(const uint8_t* buffer, int length, QDeadlineTimer deadline)
{
    WriteContext context;
    context.transportDeadline = deadline;
    context.hasTransportDeadline = true;
    return _write(QByteArray(reinterpret_cast<const char*>(buffer), length), context);
}

GPSReadResult ScriptedReceiver::_read(uint8_t* buffer, int length, int timeoutMs, std::optional<GPSDeadline> deadline)
{
    if (_readHandler) {
        if (const auto result = _readHandler(buffer, length, timeoutMs)) {
            return *result;
        }
    }
    if (_nextReadResult) {
        auto result = *_nextReadResult;
        _nextReadResult.reset();
        return result;
    }
    if (isCancelled()) {
        return {GPSReadStatus::Cancelled};
    }
    if (_readSteps.empty() && _model) {
        if (deadline) {
            _model->onProtocolReadWait(*this, *deadline);
        } else {
            _model->onTransportReadWait(*this, timeoutMs);
        }
    }
    if (_nextReadResult) {
        auto result = *_nextReadResult;
        _nextReadResult.reset();
        return result;
    }
    if (isCancelled()) {
        return {GPSReadStatus::Cancelled};
    }
    if (_readSteps.empty()) {
        return {GPSReadStatus::TimedOut};
    }
    return _readOne(buffer, length);
}

GPSWriteResult ScriptedReceiver::_write(const QByteArray& bytes, const WriteContext& context)
{
    if (_writeHandler) {
        if (const auto result = _writeHandler(bytes, context)) {
            return *result;
        }
    }
    if (_nextWriteResult) {
        auto result = *_nextWriteResult;
        _nextWriteResult.reset();
        return result;
    }
    if (!_model) {
        return {GPSWriteStatus::Unsupported};
    }

    _pendingWrites += bytes;
    while (true) {
        auto command = _model->takeCommand(_pendingWrites);
        if (!command) {
            break;
        }
        _commands.append(*command);
        if (baudrateMatches()) {
            const auto result = _model->handleCommand(*this, *command, context);
            if (result.status != GPSWriteStatus::Completed || result.acceptedBytes != command->size() ||
                result.writtenBytes != command->size()) {
                return result;
            }
        }
    }
    const int length = bytes.size();
    return {GPSWriteStatus::Completed, length, length};
}

GPSReadResult ScriptedReceiver::_readOne(uint8_t* buffer, int length)
{
    while (!_readSteps.empty() && _readSteps.front().options.drop) {
        if (_readSteps.front().options.onConsumed) {
            _readSteps.front().options.onConsumed();
        }
        _readSteps.pop_front();
    }
    if (_readSteps.empty()) {
        return {GPSReadStatus::TimedOut};
    }
    if (!buffer || length <= 0) {
        return {GPSReadStatus::InvalidData};
    }

    int copied = 0;
    do {
        ReadStep& step = _readSteps.front();
        if (step.options.delayMs > 0) {
            QThread::msleep(static_cast<unsigned long>(step.options.delayMs));
            step.options.delayMs = 0;
        }
        const int available = static_cast<int>(step.bytes.size() - step.offset);
        int count = std::min(length - copied, available);
        if (step.options.maxChunkSize > 0) {
            count = std::min(count, step.options.maxChunkSize);
        } else if (_model) {
            count = std::min(count, _model->readChunkSize(*this, length - copied, available));
        }
        if (count <= 0) {
            return copied > 0 ? GPSReadResult{GPSReadStatus::Data, copied} : GPSReadResult{GPSReadStatus::TimedOut};
        }
        std::memcpy(buffer + copied, step.bytes.constData() + step.offset, static_cast<size_t>(count));
        copied += count;
        step.offset += count;
        const bool consumed = step.offset == step.bytes.size();
        const bool coalesce = step.options.coalesce || (_model && _model->coalesceReads(*this));
        if (consumed) {
            if (step.options.onConsumed) {
                step.options.onConsumed();
            }
            _readSteps.pop_front();
        }
        if (!coalesce || !consumed) {
            break;
        }
    } while (copied < length && !_readSteps.empty());

    return copied > 0 ? GPSReadResult{GPSReadStatus::Data, copied} : GPSReadResult{GPSReadStatus::TimedOut};
}

void ScriptedReceiver::_attachModel(Model* model)
{
    _model = model;
    if (_model) {
        _model->reset(*this);
    }
}
