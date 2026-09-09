#include "GPSReceiverSession.h"

#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSReceiverSessionLog, "GPS.Receiver.GPSReceiverSession")

GPSReceiverSession::GPSReceiverSession(QObject* parent)
    : QObject(parent)
{
    qCDebug(GPSReceiverSessionLog) << this;
}

GPSReceiverSession::~GPSReceiverSession()
{
    qCDebug(GPSReceiverSessionLog) << this;
    disconnect(this, nullptr, nullptr, nullptr);
    stop();
}

void GPSReceiverSession::start(GPSType type, GPSProvider::TransportFactory factory, const GPSReceiverConfig& config)
{
    if (_shutdown) {
        return;
    }
    const QPointer<GPSReceiverSession> lifetime(this);
    stop();
    if (!lifetime || _shutdown || _provider) {
        return;
    }
    _config = config;
    const quint64 generation = ++_generation;
    _errorDetail.clear();
    _capabilities = GPSReceiverCapabilities::forType(type);
    if (config.outputProtocol == GPSReceiverConfig::OutputProtocol::NMEA) {
        _nmeaStream = std::make_unique<GPSByteStream>();
    }
    auto* worker =
        new GPSProvider(std::move(factory), type, config, _nmeaStream ? _nmeaStream->buffer() : nullptr, this);
    _provider = worker;
    _workers.insert(worker);
    const QPointer<GPSProvider> current(worker);
    const auto isCurrent = [this, current, generation]() {
        return current && _provider == current && _generation == generation;
    };
    if (_nmeaStream) {
        connect(worker, &GPSProvider::nmeaDataReady, _nmeaStream.get(), &GPSByteStream::notifyReadyRead,
                Qt::QueuedConnection);
    }
    connect(worker, &QObject::destroyed, this, [this, worker]() {
        _workers.remove(worker);
        _started.remove(worker);
        if (_retiring.remove(worker)) {
            emit stateChanged();
        }
    });
    connect(
        worker, &GPSProvider::sensorGpsUpdate, this,
        [this, isCurrent, generation](GPSObservation observation) {
            if (isCurrent()) {
                observation.sessionId = generation;
                emit positionReceived(observation);
            }
        },
        Qt::QueuedConnection);
    connect(
        worker, &GPSProvider::satelliteInfoUpdate, this,
        [this, isCurrent, generation](GPSSatelliteObservation observation) {
            if (isCurrent()) {
                observation.sessionId = generation;
                emit satellitesReceived(observation);
            }
        },
        Qt::QueuedConnection);
    connect(
        worker, &GPSProvider::relativePositionUpdate, this,
        [this, isCurrent, generation](GPSRelativeObservation observation) {
            if (isCurrent()) {
                observation.sessionId = generation;
                emit relativePositionReceived(observation);
            }
        },
        Qt::QueuedConnection);
    connect(
        worker, &GPSProvider::RTCMDataUpdate, this,
        [this, isCurrent](const QByteArray& data) {
            if (isCurrent()) {
                emit rtcmReceived(data);
            }
        },
        Qt::QueuedConnection);
    connect(
        worker, &GPSProvider::RTCMFrameUpdate, this,
        [this, isCurrent](const QByteArray& data, qint64 receivedAtMs) {
            if (isCurrent()) {
                emit rtcmFrameReceived(data, receivedAtMs);
            }
        },
        Qt::QueuedConnection);
    connect(
        worker, &GPSProvider::capabilitiesUpdated, this,
        [this, isCurrent](const GPSReceiverCapabilities& capabilities) {
            if (isCurrent()) {
                _capabilities = capabilities;
                emit capabilitiesUpdated(capabilities);
            }
        },
        Qt::QueuedConnection);
    connect(
        worker, &GPSProvider::connectionErrorDetail, this,
        [this, isCurrent](GPSConnectionError error, const QString& detail) {
            if (isCurrent()) {
                _errorDetail = detail;
                emit connectionErrorDetail(error, detail);
            }
        },
        Qt::QueuedConnection);
    connect(
        worker, &GPSProvider::surveyInStatus, this,
        [this, isCurrent](const GPSSurveyInStatus& status) {
            if (isCurrent()) {
                emit surveyInReceived(status);
            }
        },
        Qt::QueuedConnection);
    connect(
        worker, &GPSProvider::transportOpened, this,
        [this, isCurrent]() {
            if (isCurrent()) {
                emit configurationStarted();
            }
        },
        Qt::QueuedConnection);
    connect(
        worker, &GPSProvider::receiverReady, this,
        [this, isCurrent]() {
            if (isCurrent()) {
                _ready = true;
                emit receiverReady();
            }
        },
        Qt::QueuedConnection);
    connect(
        worker, &GPSProvider::connectionError, this,
        [this, isCurrent](GPSConnectionError error) {
            if (isCurrent()) {
                _ready = false;
                const QPointer<GPSReceiverSession> guard(this);
                emit disconnected();
                if (guard && isCurrent()) {
                    emit connectionError(error);
                }
            }
        },
        Qt::QueuedConnection);
    connect(
        worker, &QThread::finished, this,
        [this, current, worker]() {
            _retiring.remove(worker);
            const QPointer<GPSReceiverSession> guard(this);
            if (current && _provider == current) {
                _provider = nullptr;
                _ready = false;
                emit disconnected();
            }
            if (guard) {
                emit stateChanged();
            }
        },
        Qt::QueuedConnection);
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    emit receiverTypeChanged(type);
    if (!lifetime || !current || !isCurrent() || _shutdown) {
        return;
    }
    _started.insert(worker);
    worker->start();
    emit stateChanged();
}

void GPSReceiverSession::stop()
{
    const QPointer<GPSReceiverSession> guard(this);
    ++_generation;
    GPSProvider* worker = _provider;
    _provider = nullptr;
    _ready = false;
    if (worker) {
        worker->stop();
        if (_started.contains(worker)) {
            _retiring.insert(worker);
            worker->setParent(nullptr);
        } else {
            // A synchronous start notification may cancel before QThread::start().
            delete worker;
        }
    }
    if (!guard) {
        return;
    }
    _nmeaStream.reset();
    if (!guard) {
        return;
    }
    emit disconnected();
    if (guard && worker) {
        emit stateChanged();
    }
}

void GPSReceiverSession::shutdown()
{
    if (_shutdown) {
        return;
    }
    _shutdown = true;
    const QPointer<GPSReceiverSession> guard(this);
    stop();
    if (!guard) {
        return;
    }
    const auto workers = _workers;
    for (GPSProvider* worker : workers) {
        worker->stop();
    }
    for (GPSProvider* worker : workers) {
        if (worker->wait()) {
            delete worker;
        } else {
            qCWarning(GPSReceiverSessionLog) << "Cannot join GPS worker during shutdown";
        }
    }
}
