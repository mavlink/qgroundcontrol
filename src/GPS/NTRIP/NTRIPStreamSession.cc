#include "NTRIPStreamSession.h"

#include <utility>

#include <QtCore/QUrl>

#include "GPSCorrectionManager.h"
#include "NTRIPConfiguration.h"
#include "NTRIPConnectionStats.h"
#include "NTRIPGgaProvider.h"
#include "NTRIPTransport.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NTRIPStreamSessionLog, "GPS.NTRIP.NTRIPStreamSession")

namespace {

QString sourceInstance(const NTRIPConnectionConfig& connection)
{
    QUrl endpoint;
    endpoint.setScheme(connection.useTls ? QStringLiteral("ntrips") : QStringLiteral("ntrip"));
    endpoint.setHost(connection.host);
    endpoint.setPort(connection.port);
    endpoint.setPath(QLatin1Char('/') + connection.mountpoint);
    return endpoint.toString(QUrl::FullyEncoded);
}

}  // namespace

NTRIPStreamSession::NTRIPStreamSession(NTRIPGgaProvider& gga, NTRIPConnectionStats& stats, QObject* parent)
    : QObject(parent)
    , _gga(gga)
    , _stats(stats)
{}

void NTRIPStreamSession::open(const NTRIPConnectionConfig& connection, GPSCorrectionManager* corrections,
                              const TransportFactory& createTransport)
{
    const auto operation = _revision.current(this);
    _stats.reset();
    if (!operation.isCurrent()) {
        return;
    }

    _transport = createTransport();
    const QPointer<NTRIPTransport> transport = _transport;
    const QPointer<GPSCorrectionManager> correctionManager = corrections;
    if (correctionManager) {
        auto registration = correctionManager->registerSource(GPSCorrectionSource::Ntrip, sourceInstance(connection));
        if (!operation.isCurrent() || !transport) {
            return;
        }
        _registration = std::move(registration);
    }
    _registered = !correctionManager.isNull();
    const auto source = _registration.weak();

    // Error handling may retire the emitting transport.
    connect(
        transport, &NTRIPTransport::error, this,
        [this](const NTRIPFailure& failure) {
            if (_transport) {
                emit failed(failure);
            }
        },
        Qt::QueuedConnection);

    // Handshake state must precede subsequently queued errors.
    connect(transport, &NTRIPTransport::connected, this, [this]() {
        if (_isCurrent()) {
            emit connected();
        }
    });

    connect(
        transport, &NTRIPTransport::correctionFrameReceived, this,
        [this, correctionManager, source](const RTCMDecodedFrame& frame) {
            _onCorrectionFrame(correctionManager, source, frame);
        },
        Qt::QueuedConnection);

    connect(transport, &NTRIPTransport::plaintextCredentialsWarning, this, [this]() {
        if (_transport) {
            emit plaintextCredentialsWarning();
        }
    });

    transport->start();
    qCDebug(NTRIPStreamSessionLog) << "NTRIP transport started";
}

void NTRIPStreamSession::startStreaming()
{
    const auto operation = _revision.current(this);
    _gga.start(_transport);
    if (operation.isCurrent()) {
        _stats.start();
    }
}

void NTRIPStreamSession::closeTransport()
{
    _revision.invalidate();
    _closeTransport();
}

bool NTRIPStreamSession::stop()
{
    const auto operation = _revision.advance(this);
    _closeTransport();
    if (!operation.isCurrent()) {
        return false;
    }
    _gga.stop();
    if (!operation.isCurrent()) {
        return false;
    }
    _stats.stop();
    return operation.isCurrent();
}

void NTRIPStreamSession::setRtcmWhitelist(const QVector<int>& messageIds)
{
    if (_transport) {
        _transport->setRtcmWhitelist(messageIds);
    }
}

void NTRIPStreamSession::_closeTransport()
{
    const QPointer<NTRIPTransport> transport = std::exchange(_transport, {});
    _registration.reset();
    if (!transport) {
        return;
    }
    transport->disconnect(this);
    transport->stop();
    if (transport) {
        transport->deleteLater();
    }
}

void NTRIPStreamSession::_onCorrectionFrame(const QPointer<GPSCorrectionManager>& corrections,
                                            const GPSCorrectionSourceRegistration::Weak& source,
                                            const RTCMDecodedFrame& frame)
{
    const auto operation = _revision.current(this);
    if (corrections) {
        corrections->acceptIngress(source.event(frame));
    }
    if (!operation.isCurrent() || !_isCurrent() || !frame.valid || frame.filtered) {
        return;
    }
    _stats.recordMessage(frame.data.size(), frame.messageId, frame.receivedAtMs);
    if (operation.isCurrent()) {
        emit rtcmReceived(frame);
    }
}

bool NTRIPStreamSession::_isCurrent() const
{
    return _transport && (!_registered || _registration.valid());
}
