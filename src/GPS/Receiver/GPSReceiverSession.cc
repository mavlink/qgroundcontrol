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
        worker, &GPSProvider::dataReady, this,
        [this, mailbox = worker->mailbox(), generation]() { _drain(mailbox, generation); }, Qt::QueuedConnection);
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

bool GPSReceiverSession::readyForCorrections() const
{
    return _ready && _provider && !_shutdown && _config.role == GPSReceiverConfig::Role::Position &&
           _capabilities.correctionInput == GPSReceiverCapabilities::Support::Supported;
}

bool GPSReceiverSession::submitCorrections(const QByteArray& data, qint64 receivedAtMs, quint64 sessionId)
{
    if (sessionId != _generation || !readyForCorrections()) {
        return false;
    }
    return _provider->mailbox()->submitCorrection(data, receivedAtMs, GPSObservation::monotonicNowUs() / 1000);
}

void GPSReceiverSession::clearPendingCorrections()
{
    if (_provider) {
        _provider->mailbox()->clearCommands();
    }
}

GPSReceiverMailbox::Stats GPSReceiverSession::deliveryStats() const
{
    return _provider ? _provider->mailbox()->stats() : GPSReceiverMailbox::Stats{};
}

void GPSReceiverSession::_drain(const std::shared_ptr<GPSReceiverMailbox>& mailbox, quint64 generation)
{
    const QPointer<GPSReceiverSession> guard(this);
    const auto current = [&]() {
        return guard && _provider && _generation == generation && _provider->mailbox() == mailbox;
    };
    if (!current()) {
        return;
    }
    auto batch = mailbox->take(GPSObservation::monotonicNowUs() / 1000);
    if (batch.position) {
        batch.position->sessionId = generation;
        emit positionReceived(*batch.position);
    }
    if (!current()) {
        return;
    }
    if (batch.satellites) {
        batch.satellites->sessionId = generation;
        emit satellitesReceived(*batch.satellites);
    }
    if (!current()) {
        return;
    }
    if (batch.relativePosition) {
        batch.relativePosition->sessionId = generation;
        emit relativePositionReceived(*batch.relativePosition);
    }
    if (!current()) {
        return;
    }
    if (batch.survey) {
        emit surveyInReceived(*batch.survey);
    }
    if (!current()) {
        return;
    }
    for (const auto& frame : batch.corrections) {
        emit rtcmFrameReceived(frame.data, frame.receivedAtMs);
        if (!current()) {
            return;
        }
        emit rtcmReceived(frame.data);
        if (!current()) {
            return;
        }
    }
    if (batch.more) {
        QMetaObject::invokeMethod(
            this, [this, mailbox, generation]() { _drain(mailbox, generation); }, Qt::QueuedConnection);
    }
}
