#include "GPSReceiverSession.h"

#include <utility>

#include "GPSRecordingTransport.h"
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

void GPSReceiverSession::start(const GPSReceiverProfile& profile, GPSProvider::TransportFactory factory)
{
    if (_shutdown) {
        return;
    }
    const auto requestedProfile = std::make_shared<const GPSReceiverProfile>(profile.normalized());
    const QPointer<GPSReceiverSession> lifetime(this);
    const quint64 replacementGeneration = _generation + 1;
    stop();
    if (!lifetime || _shutdown || _provider || _generation != replacementGeneration) {
        return;
    }
    const quint64 generation = ++_generation;
    _attempt = {generation, requestedProfile, GPSReceiverAttempt::Phase::Connecting, GPSConnectionError::None, {}};
    const auto type = _attempt.profile->driverType;
    const auto config = _attempt.profile->receiver;
    _configurationTerminal = false;
    _capabilities = GPSReceiverCapabilities::forType(type);
    if (config.outputProtocol == GPSReceiverConfig::OutputProtocol::NMEA) {
        _nmeaStream = std::make_unique<GPSByteStream>();
    }
    std::shared_ptr<GPSRecordingStream> recording;
    if (_recordingBuffer && factory) {
        recording = std::make_shared<GPSRecordingStream>(_recordingBuffer,
                                                         GPSRecordingMetadata::fromProfile(*_attempt.profile));
        factory = [factory = std::move(factory), recording](const std::atomic_bool& stop) {
            return std::make_unique<GPSRecordingTransport>(factory(stop), stop, recording);
        };
    }
    auto* worker =
        new GPSProvider(std::move(factory), type, config, _nmeaStream ? _nmeaStream->buffer() : nullptr, this);
    worker->setRecordingStream(recording);
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
        worker, &GPSProvider::transportOpenFinished, this,
        [this, isCurrent](const GPSOpenResult& result) {
            if (isCurrent() && !_attempt.terminal() && !_attempt.transportOpen) {
                _attempt.transportOpen = result;
                const auto snapshot = _attempt;
                emit attemptChanged(snapshot);
            }
        },
        Qt::QueuedConnection);
    connect(
        worker, &GPSProvider::configurationFinished, this,
        [this, isCurrent](const GPSConfigurationResult& result) {
            if (isCurrent() && !_attempt.terminal() && !_attempt.configurationResult) {
                _attempt.configurationResult = result;
                const auto snapshot = _attempt;
                emit attemptChanged(snapshot);
            }
        },
        Qt::QueuedConnection);
    connect(
        worker, &GPSProvider::transportReadFailed, this,
        [this, isCurrent](const GPSReadResult& result) {
            if (isCurrent() && !_attempt.terminal() && !_attempt.transportRead) {
                _attempt.transportRead = result;
                const auto snapshot = _attempt;
                emit attemptChanged(snapshot);
            }
        },
        Qt::QueuedConnection);
    connect(
        worker, &GPSProvider::configurationReported, this,
        [this, isCurrent, generation](const GPSConfigurationReport& report) {
            if (isCurrent() && !_configurationTerminal) {
                _configurationReport = report;
                _configurationReport.sessionId = generation;
                _configurationReport.active = true;
                emit configurationReported(_configurationReport);
            }
        },
        Qt::QueuedConnection);
    connect(
        worker, &GPSProvider::connectionErrorDetail, this,
        [this, isCurrent](GPSConnectionError error, const QString& detail) {
            if (isCurrent() && !_attempt.terminal()) {
                _attempt.error = error;
                _attempt.errorDetail = detail;
                emit connectionErrorDetail(error, detail);
            }
        },
        Qt::QueuedConnection);
    connect(
        worker, &GPSProvider::transportOpened, this,
        [this, isCurrent]() {
            if (isCurrent() && _transition(GPSReceiverAttempt::Phase::Configuring) && isCurrent()) {
                emit configurationStarted();
            }
        },
        Qt::QueuedConnection);
    connect(
        worker, &GPSProvider::receiverReady, this,
        [this, isCurrent]() {
            if (isCurrent() && _transition(GPSReceiverAttempt::Phase::Ready) && isCurrent()) {
                emit receiverReady();
            }
        },
        Qt::QueuedConnection);
    connect(
        worker, &GPSProvider::connectionError, this,
        [this, isCurrent, mailbox = worker->mailbox(), generation](GPSConnectionError error) {
            if (isCurrent()) {
                const QPointer<GPSReceiverSession> guard(this);
                _flushDeliveries(mailbox, generation);
                if (!guard || !isCurrent()) {
                    return;
                }
                _finishAttempt(error);
            }
        },
        Qt::QueuedConnection);
    connect(
        worker, &QThread::finished, this,
        [this, current, worker, mailbox = worker->mailbox(), generation]() {
            _retiring.remove(worker);
            const QPointer<GPSReceiverSession> guard(this);
            if (current && _provider == current) {
                _flushDeliveries(mailbox, generation);
                if (!guard || !current || _provider != current) {
                    return;
                }
                _provider = nullptr;
                _finishAttempt();
            }
            if (guard) {
                emit stateChanged();
            }
        },
        Qt::QueuedConnection);
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    const auto snapshot = _attempt;
    emit attemptChanged(snapshot);
    if (!lifetime || !isCurrent() || _shutdown) {
        return;
    }
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
    const quint64 retiredGeneration = _generation;
    const quint64 stoppingGeneration = ++_generation;
    GPSProvider* worker = _provider;
    const auto mailbox = worker ? worker->mailbox() : nullptr;
    _provider = nullptr;
    _configurationTerminal = true;
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
    if (mailbox) {
        _flushDeliveries(mailbox, retiredGeneration);
    }
    if (!guard || _generation != stoppingGeneration) {
        return;
    }
    if (_configurationReport.sessionId != 0 || !_configurationReport.settings.isEmpty()) {
        _configurationReport = {};
        emit configurationReported(_configurationReport);
        if (!guard || _generation != stoppingGeneration) {
            return;
        }
    }
    _nmeaStream.reset();
    if (!guard || _generation != stoppingGeneration) {
        return;
    }
    _finishAttempt();
    if (guard && worker) {
        emit stateChanged();
    }
}

const GPSReceiverProfile& GPSReceiverSession::profile() const
{
    static const GPSReceiverProfile empty;
    return _attempt.profile ? *_attempt.profile : empty;
}

bool GPSReceiverSession::_transition(GPSReceiverAttempt::Phase phase)
{
    if (_attempt.terminal() || _attempt.phase == phase ||
        (phase == GPSReceiverAttempt::Phase::Configuring && _attempt.phase != GPSReceiverAttempt::Phase::Connecting)) {
        return false;
    }
    const QPointer<GPSReceiverSession> guard(this);
    const quint64 generation = _attempt.generation;
    _attempt.phase = phase;
    if (phase == GPSReceiverAttempt::Phase::Ready) {
        _attempt.error = GPSConnectionError::None;
        _attempt.errorDetail.clear();
    }
    const auto snapshot = _attempt;
    emit attemptChanged(snapshot);
    if (guard && _attempt.generation == generation) {
        emit stateChanged();
    }
    return guard && _attempt.generation == generation && _attempt.phase == phase;
}

void GPSReceiverSession::_finishAttempt(GPSConnectionError error)
{
    if (_attempt.generation == 0 || _attempt.terminal()) {
        return;
    }
    const QPointer<GPSReceiverSession> guard(this);
    const quint64 generation = _attempt.generation;
    _attempt.error = error;
    _attempt.phase =
        error == GPSConnectionError::None ? GPSReceiverAttempt::Phase::Cancelled : GPSReceiverAttempt::Phase::Failed;
    _invalidateConfigurationReport();
    if (!guard || _attempt.generation != generation) {
        return;
    }
    const auto snapshot = _attempt;
    emit attemptChanged(snapshot);
    if (!guard || _attempt.generation != generation) {
        return;
    }
    emit disconnected();
    if (!guard || _attempt.generation != generation) {
        return;
    }
    if (error != GPSConnectionError::None) {
        emit connectionError(error);
    }
    if (guard && _attempt.generation == generation) {
        emit stateChanged();
    }
}

void GPSReceiverSession::_invalidateConfigurationReport()
{
    _configurationTerminal = true;
    if (_configurationReport.active) {
        _configurationReport.active = false;
        emit configurationReported(_configurationReport);
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
    return ready() && _provider && !_shutdown && config().role == GPSReceiverConfig::Role::Position &&
           _capabilities.correctionInput == GPSReceiverCapabilities::Support::Supported;
}

bool GPSReceiverSession::submitCorrections(const QByteArray& data, qint64 receivedAtMs, quint64 sessionId)
{
    GPSCorrectionFrame frame;
    frame.data = data;
    frame.receivedAtMs = receivedAtMs;
    return submitCorrections(frame, sessionId).accepted;
}

GPSCorrectionSubmitResult GPSReceiverSession::submitCorrections(const GPSCorrectionFrame& frame, quint64 sessionId)
{
    if (sessionId != _generation) {
        return {false, GPSCorrectionOutcome::Cancelled};
    }
    if (!readyForCorrections()) {
        return {false, GPSCorrectionOutcome::NotReady};
    }
    return _provider->mailbox()->submitCorrection(frame, sessionId, GPSObservation::monotonicNowUs() / 1000);
}

void GPSReceiverSession::clearPendingCorrections()
{
    if (!_provider) {
        return;
    }
    const QPointer<GPSReceiverSession> guard(this);
    const auto mailbox = _provider->mailbox();
    const quint64 generation = _generation;
    const bool notify = mailbox->clearCommands();
    // Removing a sink may invalidate its pending IDs as soon as this call returns.
    _flushDeliveries(mailbox, generation);
    if (guard && _provider && _generation == generation && notify) {
        // Consume the reserved wakeup even when the synchronous flush emptied its reports.
        QMetaObject::invokeMethod(
            this, [this, mailbox, generation]() { _drain(mailbox, generation); }, Qt::QueuedConnection);
    }
}

void GPSReceiverSession::_flushDeliveries(const std::shared_ptr<GPSReceiverMailbox>& mailbox, quint64 generation)
{
    auto deliveries = mailbox->takeDeliveries();
    deliveries.removeIf(
        [generation](const GPSCorrectionDelivery& delivery) { return delivery.destinationSession != generation; });
    if (!deliveries.empty()) {
        emit correctionDeliveriesReady(deliveries);
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
    batch.deliveries.removeIf(
        [generation](const GPSCorrectionDelivery& delivery) { return delivery.destinationSession != generation; });
    if (!batch.deliveries.empty()) {
        emit correctionDeliveriesReady(batch.deliveries);
    }
    if (!current()) {
        return;
    }
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
        batch.survey->sessionId = generation;
        emit surveyInReceived(*batch.survey);
    }
    if (!current()) {
        return;
    }
    for (const auto& frame : batch.corrections) {
        emit rtcmFrameReceived(frame.data, frame.receivedAtMs, generation);
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
