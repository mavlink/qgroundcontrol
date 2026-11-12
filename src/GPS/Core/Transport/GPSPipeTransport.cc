#include "GPSPipeTransport.h"

#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSPipeTransportLog, "GPS.Transport.GPSPipeTransport")

GPSPipeTransport::GPSPipeTransport(std::shared_ptr<GPSPipe> pipe, GPSCancelToken cancelToken, Timeouts timeouts,
                                   GPSClock clock)
    : GPSTransport(std::move(cancelToken))
    , _pipe(std::move(pipe))
    , _timeouts(timeouts)
    , _clock(std::move(clock))
    , _wakeOnCancel(this->cancelToken(), [session = _pipe]() { session->_wakeSession(); })
{}

GPSPipeTransport::GPSPipeTransport(std::shared_ptr<GPSPipe> pipe, GPSCancelToken cancelToken)
    : GPSPipeTransport(std::move(pipe), std::move(cancelToken), Timeouts{})
{}

GPSPipeTransport::~GPSPipeTransport()
{
    _pipe->_closeSession();
}

std::function<std::unique_ptr<GPSTransport>(GPSCancelToken)> GPSPipeTransport::factory(std::shared_ptr<GPSPipe> pipe,
                                                                                       Timeouts timeouts)
{
    // Counts the factory's copies and its transport apart from the pipe's other owners.
    GPSPipe* const session = pipe.get();
    const std::shared_ptr<GPSPipe> sessionSide(session,
                                               [pipe = std::move(pipe)](GPSPipe* ended) { ended->_closeSession(); });
    return [sessionSide, timeouts](GPSCancelToken cancelToken) -> std::unique_ptr<GPSTransport> {
        return std::make_unique<GPSPipeTransport>(sessionSide, std::move(cancelToken), timeouts);
    };
}

GPSOpenResult GPSPipeTransport::open()
{
    GPSOpenResult result = _pipe->_open({cancelToken(), _clock}, _timeouts.open);
    if (result.status != GPSOpenStatus::Opened) {
        qCDebug(GPSPipeTransportLog) << "The receiver's link did not open:" << result.detail;
        // The link's owner stops bringing up a link nothing waits for.
        _pipe->_closeSession();
    }
    return result;
}

bool GPSPipeTransport::fatalError() const
{
    return _pipe->_fatal();
}

GPSReadResult GPSPipeTransport::read(std::span<uint8_t> buffer, std::chrono::milliseconds timeout)
{
    return _pipe->_read({cancelToken(), _clock}, buffer, timeout);
}

GPSWriteResult GPSPipeTransport::writeData(QByteArrayView bytes, QDeadlineTimer deadline)
{
    return _pipe->_write({cancelToken(), _clock}, bytes, deadline);
}
