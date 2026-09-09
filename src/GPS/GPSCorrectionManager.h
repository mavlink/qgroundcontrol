#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtCore/QVariantList>
#include <QtQmlIntegration/QtQmlIntegration>

#include "GPSCorrectionFrame.h"
#include "GPSCorrectionRouter.h"
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
    Q_PROPERTY(QVariantList sourceInstances READ sourceInstances NOTIFY sourcesChanged)
    Q_PROPERTY(QString activeInstance READ activeInstance NOTIFY sourcesChanged)

    friend class GPSCorrectionManagerTest;

public:
    enum class RoutingPolicy
    {
        Automatic,
        Manual,
        All
    };
    Q_ENUM(RoutingPolicy)

    explicit GPSCorrectionManager(QObject* parent = nullptr);
    ~GPSCorrectionManager() override;

    void init(NTRIPSettings* settings);
    void shutdown();

    RTCMMavlink* rtcmMavlink() { return &_rtcmMavlink; }

    quint64 beginSourceSession(GPSCorrectionSource source, const QString& instance = {});
    void endSourceSession(GPSCorrectionSource source);
    quint64 sourceSession(GPSCorrectionSource source) const;
    void setSelectedSource(GPSCorrectionSource source);

    GPSCorrectionSource selectedSource() const { return _router.selectedSource(); }

    void setRoutingPolicy(RoutingPolicy policy);
    RoutingPolicy routingPolicy() const;
    void setSelectedInstance(const QString& instance);

    QString activeInstance() const { return _router.activeInstance(); }

    void addSink(const QString& id, GPSCorrectionRouter::Sink sink);
    void removeSink(const QString& id);

    QVariantList sources() const;
    QVariantList sourceInstances() const;
    void forwardCorrectionsFrom(GPSCorrectionSource source, const QByteArray& data, bool validated = false,
                                int messageId = 0, bool filtered = false);
    void acceptFrame(const GPSCorrectionFrame& frame);

signals:
    void sourcesChanged();
    void correctionRouted(const GPSCorrectionFrame& frame);
    void selectedSourceChanged();

public slots:
    void forwardCorrections(const QByteArray& data);

private:
    void _applyUdpInputSettings();

    void _scheduleSourcesChanged();

    GPSCorrectionRouter _router;
    QTimer _diagnosticsTimer;
    QTimer _healthTimer;
    RTCMMavlink _rtcmMavlink;
    RTCMUdpInput _udpInput;
    QPointer<NTRIPSettings> _settings;
    bool _shutdown = false;
};
