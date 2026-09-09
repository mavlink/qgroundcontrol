#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtCore/QVariantList>
#include <QtQmlIntegration/QtQmlIntegration>

#include <array>

#include "GPSCorrectionFrame.h"
#include "RTCMMavlink.h"
#include "RTCMUdpInput.h"

class NTRIPSettings;

/// Owns the shared MAVLink sequence domain and UDP correction input for all GPS sources.
class GPSCorrectionManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(RTCMMavlink* rtcmMavlink READ rtcmMavlink CONSTANT)
    Q_PROPERTY(QVariantList sources READ sources NOTIFY sourcesChanged)

    friend class GPSCorrectionManagerTest;

public:
    explicit GPSCorrectionManager(QObject* parent = nullptr);
    ~GPSCorrectionManager() override;

    void init(NTRIPSettings* settings);
    void shutdown();

    RTCMMavlink* rtcmMavlink() { return &_rtcmMavlink; }

    quint64 beginSourceSession(GPSCorrectionSource source);
    void endSourceSession(GPSCorrectionSource source);
    quint64 sourceSession(GPSCorrectionSource source) const;
    void setSelectedSource(GPSCorrectionSource source);

    GPSCorrectionSource selectedSource() const { return _selectedSource; }

    QVariantList sources() const;
    void forwardCorrectionsFrom(GPSCorrectionSource source, const QByteArray& data, bool validated = false,
                                int messageId = 0, bool filtered = false);
    void acceptFrame(const GPSCorrectionFrame& frame);

signals:
    void sourcesChanged();
    void correctionRouted(const GPSCorrectionFrame& frame);

public slots:
    void forwardCorrections(const QByteArray& data);

private:
    void _applyUdpInputSettings();

    struct SourceStats
    {
        quint64 session = 1;
        bool active = false;
        quint64 receivedBytes = 0;
        quint64 validatedFrames = 0;
        quint64 filteredFrames = 0;
        quint64 routedFrames = 0;
        quint64 submittedBytes = 0;
        qint64 lastValidMs = 0;
    };

    static constexpr qint64 FRESHNESS_TIMEOUT_MS = 5000;
    static int _sourceIndex(GPSCorrectionSource source);

    std::array<SourceStats, 4> _sources;
    GPSCorrectionSource _selectedSource = GPSCorrectionSource::Unknown;
    QTimer _healthTimer;
    RTCMMavlink _rtcmMavlink;
    RTCMUdpInput _udpInput;
    QPointer<NTRIPSettings> _settings;
    bool _shutdown = false;
};
