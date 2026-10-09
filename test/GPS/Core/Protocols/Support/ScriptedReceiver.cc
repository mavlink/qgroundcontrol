#include "ScriptedReceiver.h"

#include <algorithm>
#include <cstring>
#include <utility>

#include "GPSProtocolRuntime.h"

namespace GPSTest {

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

std::optional<QByteArray> ScriptedReceiver::Model::takeLine(QByteArray& pending)
{
    const qsizetype end = pending.indexOf("\r\n");
    if (end < 0) {
        return std::nullopt;
    }
    QByteArray command = pending.first(end + 2);
    pending.remove(0, end + 2);
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

void ScriptedReceiver::Model::onTransportReadWait(ScriptedReceiver& receiver, std::chrono::milliseconds timeout)
{
    Q_UNUSED(receiver)
    Q_UNUSED(timeout)
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

ScriptedReceiver::ScriptedReceiver(GPSCancelToken cancelToken, Model* model)
    : GPSTransport(std::move(cancelToken))
{
    _attachModel(model);
}

ScriptedReceiver::ScriptedReceiver(GPSCancelSource cancelSource, Model* model)
    : GPSTransport(cancelSource.token())
    , _cancelSource(std::move(cancelSource))
{
    _attachModel(model);
}

ScriptedReceiver::~ScriptedReceiver() = default;

GPSOpenResult ScriptedReceiver::open()
{
    ++_opens;
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
    return {GPSOpenStatus::Opened};
}

bool ScriptedReceiver::setBaudrate(unsigned baudrate)
{
    return _setBaudrate(baudrate);
}

std::chrono::milliseconds ScriptedReceiver::configurationWriteTimeout() const
{
    if (_configurationWriteTimeoutHandler) {
        return _configurationWriteTimeoutHandler();
    }
    return GPSTransport::configurationWriteTimeout();
}

GPSReadResult ScriptedReceiver::read(std::span<uint8_t> buffer, std::chrono::milliseconds timeout)
{
    if (!_clock) {
        return _read(buffer, timeout, std::nullopt);
    }
    const GPSDeadline deadline = GPSDeadline::after(_clock->nowUs(), timeout);
    const GPSReadResult result = _read(buffer, timeout, deadline);
    if (result.status == GPSReadStatus::TimedOut) {
        _clock->advanceTo(deadline.untilUs);
    }
    return result;
}

GPSRuntimeIO ScriptedReceiver::makeIO(GPSRuntimeIO io)
{
    io.read = [this](std::span<uint8_t> bytes, GPSDeadline deadline) {
        return _read(bytes, std::chrono::milliseconds::zero(), deadline);
    };
    io.write = [this](std::span<const uint8_t> bytes, GPSDeadline deadline) {
        WriteContext context;
        context.protocolDeadline = deadline;
        context.hasProtocolDeadline = true;
        return _write(QByteArrayView(bytes).toByteArray(), context);
    };
    io.setBaudrate = [this](unsigned baudrate) {
        return _setBaudrate(baudrate) ? GPSBaudStatus::Configured : GPSBaudStatus::Unsupported;
    };
    return io;
}

void ScriptedReceiver::setModel(Model* model)
{
    _attachModel(model);
}

void ScriptedReceiver::queueReply(const QByteArray& bytes)
{
    if (!bytes.isEmpty()) {
        _readSteps.push_back(ReadStep{.bytes = bytes});
    }
}

void ScriptedReceiver::clearReplies()
{
    _readSteps.clear();
}

void ScriptedReceiver::cancel()
{
    _cancelSource.cancel();
}

bool ScriptedReceiver::_baudrateMatches() const
{
    return !_baudrateEnforced || !_receiverBaudrate || _hostBaudrate == _receiverBaudrate;
}

GPSWriteResult ScriptedReceiver::writeData(QByteArrayView bytes, QDeadlineTimer deadline)
{
    WriteContext context;
    context.transportDeadline = deadline;
    return _write(bytes.toByteArray(), context);
}

GPSReadResult ScriptedReceiver::_read(std::span<uint8_t> buffer, std::chrono::milliseconds timeout,
                                      std::optional<GPSDeadline> deadline)
{
    ++_reads;
    if (_readHandler) {
        if (const auto result = _readHandler(buffer.data(), static_cast<int>(buffer.size()), timeout)) {
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
            _model->onTransportReadWait(*this, timeout);
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
    return _readOne(buffer);
}

GPSWriteResult ScriptedReceiver::_write(const QByteArray& bytes, const WriteContext& context)
{
    ++_writes;
    if (_writeHandler) {
        if (const auto result = _writeHandler(bytes, context)) {
            return *result;
        }
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
        if (_baudrateMatches()) {
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

bool ScriptedReceiver::_setBaudrate(unsigned baudrate)
{
    ++_baudChanges;
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
    return true;
}

GPSReadResult ScriptedReceiver::_readOne(std::span<uint8_t> buffer)
{
    if (_readSteps.empty()) {
        return {GPSReadStatus::TimedOut};
    }
    if (buffer.empty()) {
        return {GPSReadStatus::Data};
    }
    const auto length = static_cast<int>(buffer.size());

    int copied = 0;
    do {
        ReadStep& step = _readSteps.front();
        const int available = static_cast<int>(step.bytes.size() - step.offset);
        int count = std::min(length - copied, available);
        if (_model) {
            count = std::min(count, _model->readChunkSize(*this, length - copied, available));
        }
        if (count <= 0) {
            return copied > 0 ? GPSReadResult{GPSReadStatus::Data, copied} : GPSReadResult{GPSReadStatus::TimedOut};
        }
        std::memcpy(buffer.data() + copied, step.bytes.constData() + step.offset, static_cast<size_t>(count));
        copied += count;
        step.offset += count;
        const bool consumed = step.offset == step.bytes.size();
        const bool coalesce = _model && _model->coalesceReads(*this);
        if (consumed) {
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

}  // namespace GPSTest
