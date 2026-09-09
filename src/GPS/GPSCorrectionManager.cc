#include "GPSCorrectionManager.h"

#include "NTRIPSettings.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSCorrectionManagerLog, "GPS.GPSCorrectionManager")

GPSCorrectionManager::GPSCorrectionManager(QObject* parent)
    : QObject(parent)
    , _rtcmMavlink(this)
    , _udpInput(0, this)
{
    qCDebug(GPSCorrectionManagerLog) << this;
    _sources[static_cast<int>(GPSCorrectionSource::Unknown)].active = true;
    connect(&_udpInput, &RTCMUdpInput::correctionReceived, this,
            [this](const QByteArray& data, int messageId, bool validated) {
                forwardCorrectionsFrom(GPSCorrectionSource::Udp, data, validated, messageId);
            });
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

int GPSCorrectionManager::_sourceIndex(GPSCorrectionSource source)
{
    const int index = static_cast<int>(source);
    return index >= 0 && index < 4 ? index : -1;
}

quint64 GPSCorrectionManager::beginSourceSession(GPSCorrectionSource source)
{
    const int index = _sourceIndex(source);
    if (index < 0 || _shutdown) {
        return 0;
    }
    const quint64 next = _sources[index].session + 1;
    _sources[index] = SourceStats{};
    _sources[index].session = next;
    _sources[index].active = true;
    emit sourcesChanged();
    return next;
}

void GPSCorrectionManager::endSourceSession(GPSCorrectionSource source)
{
    const int index = _sourceIndex(source);
    if (index < 0) {
        return;
    }
    _sources[index].active = false;
    emit sourcesChanged();
}

quint64 GPSCorrectionManager::sourceSession(GPSCorrectionSource source) const
{
    const int index = _sourceIndex(source);
    return index >= 0 ? _sources[index].session : 0;
}

void GPSCorrectionManager::setSelectedSource(GPSCorrectionSource source)
{
    if (_sourceIndex(source) >= 0 && _selectedSource != source) {
        _selectedSource = source;
        emit sourcesChanged();
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
    const int index = _sourceIndex(frame.source);
    if (_shutdown || index < 0 || frame.data.isEmpty()) {
        return;
    }
    auto& stats = _sources[index];
    if (!stats.active || frame.session != stats.session) {
        return;
    }
    stats.receivedBytes += frame.data.size();
    const qint64 now = GPSCorrectionFrame::monotonicNowMs();
    if (frame.receivedAtMs <= 0 || frame.receivedAtMs > now) {
        ++stats.filteredFrames;
        emit sourcesChanged();
        return;
    }
    if (frame.validated) {
        ++stats.validatedFrames;
        stats.lastValidMs = qMax(stats.lastValidMs, frame.receivedAtMs);
    }
    const bool expired = now - frame.receivedAtMs >= FRESHNESS_TIMEOUT_MS;
    if (frame.filtered || expired ||
        (_selectedSource != GPSCorrectionSource::Unknown && _selectedSource != frame.source)) {
        ++stats.filteredFrames;
        emit sourcesChanged();
        return;
    }
    GPSCorrectionFrame routedFrame = frame;
    if (routedFrame.validated && routedFrame.messageId == 0 && routedFrame.data.size() >= 8 &&
        static_cast<quint8>(routedFrame.data[0]) == 0xD3) {
        routedFrame.messageId =
            (static_cast<quint8>(routedFrame.data[3]) << 4) | (static_cast<quint8>(routedFrame.data[4]) >> 4);
    }
    ++stats.routedFrames;
    // submit() reports bytes queued to connected links, not receiver acknowledgement.
    const QPointer<GPSCorrectionManager> guard(this);
    const quint64 submitted = _rtcmMavlink.submit(frame.data);
    if (!guard || _shutdown || _sources[index].session != frame.session) {
        return;
    }
    _sources[index].submittedBytes += submitted;
    emit correctionRouted(routedFrame);
    if (guard) {
        emit sourcesChanged();
    }
}

QVariantList GPSCorrectionManager::sources() const
{
    QVariantList result;
    const qint64 now = GPSCorrectionFrame::monotonicNowMs();
    for (int index = 0; index < static_cast<int>(_sources.size()); ++index) {
        const auto& stats = _sources[index];
        result.append(
            QVariantMap{{QStringLiteral("source"), index},
                        {QStringLiteral("session"), QVariant::fromValue(stats.session)},
                        {QStringLiteral("active"), stats.active},
                        {QStringLiteral("receivedBytes"), QVariant::fromValue(stats.receivedBytes)},
                        {QStringLiteral("validatedFrames"), QVariant::fromValue(stats.validatedFrames)},
                        {QStringLiteral("filteredFrames"), QVariant::fromValue(stats.filteredFrames)},
                        {QStringLiteral("routedFrames"), QVariant::fromValue(stats.routedFrames)},
                        {QStringLiteral("submittedBytes"), QVariant::fromValue(stats.submittedBytes)},
                        {QStringLiteral("ageMs"), stats.lastValidMs > 0 ? now - stats.lastValidMs : qint64(-1)},
                        {QStringLiteral("usable"),
                         stats.active && stats.lastValidMs > 0 && now - stats.lastValidMs < FRESHNESS_TIMEOUT_MS}});
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
    for (auto& source : _sources) {
        source.active = false;
    }
    _udpInput.stop();
    emit sourcesChanged();
}
