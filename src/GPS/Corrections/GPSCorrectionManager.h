#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtCore/QVariantList>
#include <QtQmlIntegration/QtQmlIntegration>

#include "GPSCorrectionDiagnosticsModel.h"
#include "GPSCorrectionEventModel.h"
#include "GPSCorrectionFrame.h"
#include "GPSCorrectionRouter.h"
#include "GPSNotificationQueue.h"
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
    Q_PROPERTY(GPSCorrectionDiagnosticsModel* sources READ sourceModel CONSTANT)
    Q_PROPERTY(QVariantList sourceInstances READ sourceInstances NOTIFY sourceInstancesChanged)
    Q_PROPERTY(GPSCorrectionEventModel* events READ events CONSTANT)
    Q_PROPERTY(GPSCorrectionDiagnosticsModel* destinations READ destinationModel CONSTANT)

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

    void removeSink(const QString& id);
    void setOutput(const QString& id, GPSCorrectionRouter::Output output);

    GPSCorrectionEventModel* events() { return &_eventModel; }

    GPSCorrectionDiagnosticsModel* sourceModel() { return &_sourceModel; }

    GPSCorrectionDiagnosticsModel* destinationModel() { return &_destinationModel; }

    /// Live per-category diagnostics, indexed by GPSCorrectionSource; the models publish them in batches.
    QVariantList sourceDiagnostics() const { return _router.sourceDiagnostics(); }

    QVariantList destinationDiagnostics() const { return _router.destinationDiagnostics(); }

    QVariantList sourceInstances() const { return _router.sourceInstanceDiagnostics(); }

signals:
    void sourceInstancesChanged();
    void correctionRouted(const GPSCorrectionFrame& frame);

private:
    void _applyRoutingSettings();
    void _applyUdpInputSettings();

    void _scheduleSourcesChanged();
    void _refreshDiagnostics();

    GPSCorrectionRouter _router;
    GPSCorrectionEventModel _eventModel;
    GPSCorrectionDiagnosticsModel _sourceModel{QStringLiteral("source"), this};
    GPSCorrectionDiagnosticsModel _destinationModel{QStringLiteral("destinationId"), this};
    QTimer _diagnosticsTimer;
    QTimer _healthTimer;
    RTCMMavlink _rtcmMavlink;
    RTCMUdpInput _udpInput;
    GPSCorrectionSourceRegistration _udpRegistration;
    UdpForwarder _ntripUdpOutput{this};
    QPointer<GPSCorrectionSettings> _settings;
    // Last published instances; notifications fire only when the list changes.
    QVariantList _sourceInstances;
    quint64 _udpConfigurationRevision = 0;
    int _ingressDepth = 0;
    bool _finalDiagnosticsPending = false;
    bool _shutdown = false;
    GPSNotificationQueue _notifications{this};
};
