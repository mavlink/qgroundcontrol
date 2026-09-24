#pragma once

#include <array>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtCore/QVariantList>
#include <QtQmlIntegration/QtQmlIntegration>

#include "GPSCorrectionEventModel.h"
#include "GPSCorrectionFrame.h"
#include "GPSCorrectionRouter.h"
#include "RTCMMavlink.h"
#include "RTCMUdpInput.h"
#include "UdpForwarder.h"

class GPSCorrectionSettings;

/// Owns the shared MAVLink sequence domain and UDP correction input for all GPS sources.
class GPSCorrectionManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(RTCMMavlink* rtcmMavlink READ rtcmMavlink CONSTANT)
    Q_PROPERTY(QVariantList sources READ sources NOTIFY sourcesChanged)
    Q_PROPERTY(QVariantList sourceInstances READ sourceInstances NOTIFY sourceInstancesChanged)
    Q_PROPERTY(GPSCorrectionEventModel* events READ events CONSTANT)
    Q_PROPERTY(QVariantList destinations READ destinations NOTIFY destinationsChanged)

    friend class GPSCorrectionManagerTest;

public:
    using RoutingPolicy = GPSCorrectionRouter::Policy;
    using RoutingConfiguration = GPSCorrectionRouter::Configuration;

    explicit GPSCorrectionManager(QObject* parent = nullptr);
    ~GPSCorrectionManager() override;

    void init(GPSCorrectionSettings* settings);
    void shutdown();
    void configureNtripUdpOutput(bool enabled, const QString& address, quint16 port);

    RTCMMavlink* rtcmMavlink() { return &_rtcmMavlink; }

    void applyRoutingConfiguration(const RoutingConfiguration& configuration);
    GPSCorrectionSourceRegistration registerSource(GPSCorrectionSource source, const QString& instance = {});
    void acceptIngress(const GPSCorrectionIngress& ingress);

    GPSCorrectionSource selectedSource() const { return _router.selectedSource(); }

    RoutingPolicy routingPolicy() const;

    void removeSink(const QString& id);
    void setOutput(const QString& id, GPSCorrectionRouter::Output output);

    GPSCorrectionEventModel* events() { return &_eventModel; }

    QVariantList destinations() const;

    QVariantList sources() const;
    QVariantList sourceInstances() const;

signals:
    void sourcesChanged();
    void sourceInstancesChanged();
    void destinationsChanged();
    void correctionRouted(const GPSCorrectionFrame& frame);
    void selectedSourceChanged();

private:
    void _applyRoutingSettings();
    void _applyUdpInputSettings();

    void _scheduleSourcesChanged();
    void _refreshDiagnostics();
    /// Samples received source bytes; the health timer provides the one-second cadence.
    void _updateReceivedByteRates(qint64 nowMs);
    QVariantList _sourceDiagnostics() const;

    GPSCorrectionRouter _router;
    GPSCorrectionEventModel _eventModel;
    QTimer _diagnosticsTimer;
    QTimer _healthTimer;
    RTCMMavlink _rtcmMavlink;
    RTCMUdpInput _udpInput;
    GPSCorrectionSourceRegistration _udpRegistration;
    UdpForwarder _ntripUdpOutput{this};
    QPointer<GPSCorrectionSettings> _settings;
    // Last published diagnostics; notifications fire only when a list changes.
    QVariantList _sources;
    QVariantList _sourceInstances;
    QVariantList _destinations;

    struct ReceivedBytesSample
    {
        quint64 session = 0;
        quint64 bytes = 0;
    };

    std::array<ReceivedBytesSample, 4> _receivedBytesSamples{};
    std::array<quint64, 4> _receivedByteRates{};
    qint64 _receivedBytesSampleMs = 0;
    quint64 _udpConfigurationRevision = 0;
    int _ingressDepth = 0;
    bool _finalDiagnosticsPending = false;
    bool _shutdown = false;
    GPSNotificationQueue _notifications{this};
};
