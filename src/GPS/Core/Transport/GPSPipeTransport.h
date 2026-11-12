#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>

#include <QtCore/QByteArrayView>
#include <QtCore/QDeadlineTimer>

#include "GPSCancellation.h"
#include "GPSClock.h"
#include "GPSPipe.h"
#include "GPSTransport.h"

/// GPSTransport over a GPSPipe: the blocking side of a link that another thread's event loop serves. Its waits end
/// at once on input, on the link's end and on a stop, without dispatching events. Such a link has no baud rate to
/// set, so drivers see the bridge rate, as over TCP: a serial adapter behind the link and its receiver must already
/// run at BRIDGE_BAUD_RATE. One transport serves a pipe; destroying it ends the session's side.
class GPSPipeTransport : public GPSTransport
{
public:
    struct Timeouts
    {
        /// How long open() waits for the link.
        std::chrono::milliseconds open{5000};
        /// How long a write waits for the link to confirm it: configurationWriteTimeout().
        std::chrono::milliseconds write{5000};
    };

    /// Waits run on @a clock: the steady clock, unless a test injects a virtual one, whose wait() then stands in for
    /// blocking.
    GPSPipeTransport(std::shared_ptr<GPSPipe> pipe, GPSCancelToken cancelToken, Timeouts timeouts, GPSClock clock = {});
    GPSPipeTransport(std::shared_ptr<GPSPipe> pipe, GPSCancelToken cancelToken);
    ~GPSPipeTransport() override;

    /// Creates the transport of the session over @a pipe, as GPSReceiverWorker asks for it. The session's side also
    /// ends when the last copy of the factory is destroyed, which tells the link's owner of a session that never
    /// created its transport.
    [[nodiscard]] static std::function<std::unique_ptr<GPSTransport>(GPSCancelToken)> factory(
        std::shared_ptr<GPSPipe> pipe, Timeouts timeouts);

    /// Asks the link's owner for the link and waits for it. A failed open ends the session's side.
    GPSOpenResult open() override;
    /// The link was not opened, a read or a write found it ended or overflowed, or a failed write retired it. The
    /// link's end shows only once a read or a write reported it, with its reason.
    bool fatalError() const override;

    unsigned fixedBaudRate() const override { return BRIDGE_BAUD_RATE; }

    GPSReadResult read(std::span<uint8_t> buffer, std::chrono::milliseconds timeout) override;

    std::chrono::milliseconds configurationWriteTimeout() const override { return _timeouts.write; }

protected:
    /// The counts tell what the pipe took and what the link's owner confirmed as sent.
    GPSWriteResult writeData(QByteArrayView bytes, QDeadlineTimer deadline) override;

private:
    const std::shared_ptr<GPSPipe> _pipe;
    const Timeouts _timeouts;
    const GPSClock _clock;
    /// A stop wakes the pipe's wait from the stopping thread. Declared last, so that it is destroyed first.
    const GPSCancelCallback _wakeOnCancel;
};
