#include "GPSCorrectionManager.h"

#include "NTRIPSettings.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSCorrectionManagerLog, "GPS.GPSCorrectionManager")

GPSCorrectionManager::GPSCorrectionManager(QObject* parent)
    : QObject(parent)
    , _router(this)
    , _rtcmMavlink(this)
    , _udpInput(0, this)
{
    qCDebug(GPSCorrectionManagerLog) << this;
    connect(&_router, &GPSCorrectionRouter::sourceSelected, this, &GPSCorrectionManager::selectedSourceChanged);
    connect(&_router, &GPSCorrectionRouter::sourceInvalidated, this, &GPSCorrectionManager::selectedSourceChanged);
    _router.setSink(QStringLiteral("mavlink"),
                    [this](const GPSCorrectionFrame& frame) { return _rtcmMavlink.submit(frame.data); });
    connect(&_udpInput, &RTCMUdpInput::frameReceived, this, [this](GPSCorrectionFrame frame) {
        frame.session = sourceSession(GPSCorrectionSource::Udp);
        acceptFrame(frame);
    });
    _diagnosticsTimer.setSingleShot(true);
    _diagnosticsTimer.setInterval(100);
    connect(&_diagnosticsTimer, &QTimer::timeout, this, &GPSCorrectionManager::sourcesChanged);
    _healthTimer.setInterval(1000);
    connect(&_healthTimer, &QTimer::timeout, this, &GPSCorrectionManager::sourcesChanged);
    _healthTimer.start();
}

GPSCorrectionManager::~GPSCorrectionManager()
{
    qCDebug(GPSCorrectionManagerLog) << this;
    shutdown();
}

void GPSCorrectionManager::init(NTRIPSettings* settings)
{
    if (_settings || !settings || _shutdown) {
        return;
    }
    _settings = settings;
    for (const Fact* fact :
         {settings->rtcmUdpInputEnabled(), settings->rtcmUdpInputPort(), settings->rtcmUdpValidate()}) {
        connect(fact, &Fact::rawValueChanged, this, &GPSCorrectionManager::_applyUdpInputSettings);
    }
    _applyUdpInputSettings();
}

void GPSCorrectionManager::_applyUdpInputSettings()
{
    if (!_settings || _shutdown) {
        return;
    }
    endSourceSession(GPSCorrectionSource::Udp);
    _udpInput.stop();
    _udpInput.setValidation(_settings->rtcmUdpValidate()->rawValue().toBool());
    _udpInput.setPort(static_cast<quint16>(_settings->rtcmUdpInputPort()->rawValue().toUInt()));
    if (_settings->rtcmUdpInputEnabled()->rawValue().toBool()) {
        beginSourceSession(GPSCorrectionSource::Udp);
        if (!_udpInput.start()) {
            endSourceSession(GPSCorrectionSource::Udp);
        }
    }
}

void GPSCorrectionManager::forwardCorrections(const QByteArray& data)
{
    forwardCorrectionsFrom(GPSCorrectionSource::Unknown, data);
}

quint64 GPSCorrectionManager::beginSourceSession(GPSCorrectionSource source, const QString& instance)
{
    const QPointer<GPSCorrectionManager> guard(this);
    const quint64 session = _router.beginSourceSession(source, instance);
    if (guard) {
        _scheduleSourcesChanged();
    }
    return session;
}

void GPSCorrectionManager::endSourceSession(GPSCorrectionSource source)
{
    const QPointer<GPSCorrectionManager> guard(this);
    _router.endSourceSession(source);
    if (guard) {
        _scheduleSourcesChanged();
    }
}

quint64 GPSCorrectionManager::sourceSession(GPSCorrectionSource source) const
{
    return _router.sourceSession(source);
}

void GPSCorrectionManager::setSelectedSource(GPSCorrectionSource source)
{
    _router.setSelectedSource(source, _router.selectedInstance());
    // Preserve the legacy setter's explicit select-all behavior.
    _router.setPolicy(source == GPSCorrectionSource::Unknown ? GPSCorrectionRouter::Policy::All
                                                             : GPSCorrectionRouter::Policy::Manual);
    _scheduleSourcesChanged();
}

void GPSCorrectionManager::setSelectedInstance(const QString& instance)
{
    _router.setSelectedSource(_router.selectedSource(), instance);
    _scheduleSourcesChanged();
}

void GPSCorrectionManager::setRoutingPolicy(RoutingPolicy policy)
{
    _router.setPolicy(static_cast<GPSCorrectionRouter::Policy>(policy));
    _scheduleSourcesChanged();
}

GPSCorrectionManager::RoutingPolicy GPSCorrectionManager::routingPolicy() const
{
    return static_cast<RoutingPolicy>(_router.policy());
}

void GPSCorrectionManager::addSink(const QString& id, GPSCorrectionRouter::Sink sink)
{
    _router.setSink(id, std::move(sink));
}

void GPSCorrectionManager::removeSink(const QString& id)
{
    _router.removeSink(id);
}

void GPSCorrectionManager::_scheduleSourcesChanged()
{
    if (!_shutdown && !_diagnosticsTimer.isActive()) {
        _diagnosticsTimer.start();
    }
}

void GPSCorrectionManager::forwardCorrectionsFrom(GPSCorrectionSource source, const QByteArray& data, bool validated,
                                                  int messageId, bool filtered)
{
    acceptFrame(
        {source, sourceSession(source), GPSCorrectionFrame::monotonicNowMs(), data, messageId, validated, filtered});
}

void GPSCorrectionManager::acceptFrame(const GPSCorrectionFrame& frame)
{
    const QPointer<GPSCorrectionManager> guard(this);
    GPSCorrectionFrame routed = frame;
    if (routed.sourceInstance.isEmpty()) {
        routed.sourceInstance = _router.sourceInstance(frame.source);
    }
    if (routed.validated && routed.messageId == 0 && routed.data.size() >= 8 &&
        static_cast<quint8>(routed.data[0]) == 0xD3) {
        routed.messageId = (static_cast<quint8>(routed.data[3]) << 4) | (static_cast<quint8>(routed.data[4]) >> 4);
    }
    const bool accepted = _router.acceptFrame(routed);
    if (!guard || _shutdown) {
        return;
    }
    _scheduleSourcesChanged();
    if (accepted) {
        emit correctionRouted(routed);
    }
}

QVariantList GPSCorrectionManager::sources() const
{
    QVariantList result;
    const qint64 now = _router.nowMs();
    const auto& statsBySource = _router.statistics();
    for (int index = 0; index < static_cast<int>(statsBySource.size()); ++index) {
        const auto& stats = statsBySource[index];
        result.append(QVariantMap{
            {QStringLiteral("source"), index},
            {QStringLiteral("session"), QVariant::fromValue(stats.session)},
            {QStringLiteral("active"), stats.active},
            {QStringLiteral("receivedBytes"), QVariant::fromValue(stats.receivedBytes)},
            {QStringLiteral("validatedFrames"), QVariant::fromValue(stats.validatedFrames)},
            {QStringLiteral("filteredFrames"), QVariant::fromValue(stats.filteredFrames)},
            {QStringLiteral("routedFrames"), QVariant::fromValue(stats.routedFrames)},
            {QStringLiteral("submittedBytes"), QVariant::fromValue(stats.submittedBytes)},
            {QStringLiteral("ageMs"), stats.lastValidMs > 0 ? now - stats.lastValidMs : qint64(-1)},
            {QStringLiteral("usable"), stats.active && stats.lastValidMs > 0 &&
                                           now - stats.lastValidMs < GPSCorrectionRouter::FRESHNESS_TIMEOUT_MS}});
    }
    return result;
}

QVariantList GPSCorrectionManager::sourceInstances() const
{
    QVariantList result;
    const qint64 now = _router.nowMs();
    for (const auto& source : _router.sources()) {
        const bool usable = source.lastRoutableMs > 0 && now >= source.lastRoutableMs &&
                            now - source.lastRoutableMs < GPSCorrectionRouter::FRESHNESS_TIMEOUT_MS;
        const bool selected =
            usable && (_router.policy() == GPSCorrectionRouter::Policy::All ||
                       (source.category == _router.activeSource() && source.instance == activeInstance()));
        result.append(QVariantMap{{QStringLiteral("source"), static_cast<int>(source.category)},
                                  {QStringLiteral("instanceId"), source.instance},
                                  {QStringLiteral("session"), QVariant::fromValue(source.session)},
                                  {QStringLiteral("active"), true},
                                  {QStringLiteral("usable"), usable},
                                  {QStringLiteral("selected"), selected}});
    }
    return result;
}

void GPSCorrectionManager::shutdown()
{
    if (_shutdown) {
        return;
    }
    _shutdown = true;
    _healthTimer.stop();
    _diagnosticsTimer.stop();
    _router.shutdown();
    _udpInput.stop();
    emit sourcesChanged();
}
