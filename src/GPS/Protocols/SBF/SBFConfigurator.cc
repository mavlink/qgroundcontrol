#include "SBF/SBFConfigurator.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <span>

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>

#include "GPSCommandChannel.h"
#include "GPSDeadline.h"
#include "GPSStreamDemux.h"
#include "SBF/SBFFamily.h"
#include "SBF/SBFPlan.h"

namespace {

using SBF::Plan::PROMPT_TIMEOUT;

/// The prompt reply collects in a buffer of this size, NUL terminator included; a full buffer starts over.
constexpr qsizetype PROMPT_BUFFER_SIZE = 150;
/// The connection descriptor is the first four bytes of the prompt reply, such as "USB1" in "USB1>".
constexpr qsizetype PORT_SIZE = 4;

/// Prompts the receiver and reads the connection descriptor it answers with into @a port.
GPSTask<bool> detectPort(GPSCommandChannel& channel, QByteArray& port)
{
    const uint64_t started = channel.nowUs();
    (void) co_await channel.write(SBF::Plan::PROMPT);

    QByteArray received;
    std::array<uint8_t, PROMPT_BUFFER_SIZE - 1> chunk{};
    bool detected = false;
    do {
        const auto capacity = static_cast<size_t>(PROMPT_BUFFER_SIZE - 1 - received.size());
        const int count = co_await channel.read(std::span(chunk).first(capacity), PROMPT_TIMEOUT);
        if (count < 0) {
            co_return false;
        }
        received.append(reinterpret_cast<const char*>(chunk.data()), count);
        // The reply is read as text, which ends at the first NUL.
        const qsizetype end = received.indexOf('\0');
        const QByteArrayView text = QByteArrayView(received).first(end < 0 ? received.size() : end);
        if (text.contains('>')) {
            port = text.first(std::min(text.size(), PORT_SIZE)).toByteArray();
            detected = true;
        }
        if (received.size() + 1 >= PROMPT_BUFFER_SIZE) {
            received.clear();
        }
    } while (GPSDeadline::after(started, PROMPT_TIMEOUT).untilUs > channel.nowUs() && !detected);

    if (!detected) {
        qCWarning(SBFProtocolLog) << "No COM port detected";
        co_return false;
    }
    qCDebug(SBFProtocolLog).noquote() << "Septentrio GNSS receiver COM port:" << port;
    co_return true;
}

}  // namespace

namespace SBF {

GPSTask<bool> configureBase(GPSCommandChannel& channel, GPSBaseStationConfig base, unsigned& baud)
{
    (void) co_await channel.setBaudrate(Plan::BAUD_RATE);
    baud = Plan::BAUD_RATE;
    (void) co_await channel.write(Plan::FORCE_COMMAND_INPUT);

    if (!(co_await channel.runSequence(Plan::silenceCorrectionOutput())).succeeded()) {
        co_return false;
    }
    QByteArray port;
    if (!co_await detectPort(channel, port)) {
        co_return false;
    }
    if (!(co_await channel.runSequence(Plan::port(port))).succeeded()) {
        co_return false;
    }
    channel.stream().setEnabled(GPSFrameKind::RTCM3, true);
    if (!(co_await channel.runSequence(Plan::base(port, base))).succeeded()) {
        co_return false;
    }
    co_return !channel.failed();
}

}  // namespace SBF
