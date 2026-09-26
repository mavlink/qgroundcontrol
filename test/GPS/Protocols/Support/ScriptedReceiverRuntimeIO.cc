#include "GPSRuntimeIO.h"
#include "ScriptedReceiver.h"

// Kept apart from ScriptedReceiver.cc so transport-only users (the hardware runner) need no runtime headers.
GPSRuntimeIO ScriptedReceiver::makeIO(GPSRuntimeIO io)
{
    io.read = [this](std::span<uint8_t> bytes, GPSDeadline deadline) {
        return _read(bytes.data(), static_cast<int>(bytes.size()), std::chrono::milliseconds::zero(), deadline);
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
