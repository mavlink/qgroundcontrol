#pragma once

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtQmlIntegration/QtQmlIntegration>

#include "GPSCorrectionDiagnosticsModel.h"
#include "GPSCorrectionEventModel.h"
#include "GPSCorrectionFrame.h"
#include "GPSCorrectionRouter.h"
#include "GPSNotificationQueue.h"
#include "GPSRevision.h"
#include "RTCMMavlink.h"
#include "RTCMUdpInput.h"
#include "UdpForwarder.h"

class GPSCorrectionSettings;

/// Owns the shared MAVLink sequence domain and the UDP correction input and output for all GPS sources.
class GPSCorrectionManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_PROPERTY(RTCMMavlink* rtcmMavlink READ rtcmMavlink CONSTANT)
    Q_PROPERTY(QAbstractItemModel* sources READ sourceModel CONSTANT)
    Q_PROPERTY(QList<GPSCorrectionStreamDiagnostic> sourceInstances READ sourceInstances NOTIFY sourceInstancesChanged)
    Q_PROPERTY(bool hasSelectedStream READ hasSelectedStream NOTIFY sourceInstancesChanged)
    Q_PROPERTY(GPSCorrectionStreamDiagnostic selectedStream READ selectedStream NOTIFY sourceInstancesChanged)
    Q_PROPERTY(quint64 selectedBytesPerSecond READ selectedBytesPerSecond NOTIFY selectedBytesPerSecondChanged)
    Q_PROPERTY(GPSCorrectionEventModel* events READ events CONSTANT)
    Q_PROPERTY(QAbstractItemModel* destinations READ destinationModel CONSTANT)

    friend class GPSCorrectionManagerTest;

public:
    using RoutingPolicy = GPSCorrectionRouter::Policy;
    using RoutingConfiguration = GPSCorrectionRouter::Configuration;
    using SourceModel = GPSCorrectionDiagnosticsModel<GPSCorrectionSourceDiagnostic>;
    using DestinationModel = GPSCorrectionDiagnosticsModel<GPSCorrectionDestinationDiagnostic>;

    explicit GPSCorrectionManager(QObject* parent = nullptr);
    ~GPSCorrectionManager() override;

    void init(GPSCorrectionSettings* settings);
    void shutdown();

    RTCMMavlink* rtcmMavlink() { return &_rtcmMavlink; }

    void applyRoutingConfiguration(const RoutingConfiguration& configuration);
    GPSCorrectionSourceRegistration registerSource(GPSCorrectionSource source, const QString& instance = {});
    void acceptIngress(const GPSCorrectionIngress& ingress);

    GPSCorrectionSource selectedSource() const { return _router.selectedSource(); }

    /// Read-only view of the routing core; routing changes go through the manager.
    const GPSCorrectionRouter& router() const { return _router; }

    void removeSink(const QString& id);
    void setOutput(const QString& id, GPSCorrectionRouter::Output output);

    GPSCorrectionEventModel* events() { return &_eventModel; }

    SourceModel* sourceModel() { return &_sourceModel; }

    DestinationModel* destinationModel() { return &_destinationModel; }

    /// Live per-category diagnostics, indexed by GPSCorrectionSource; the models publish them in batches.
    QList<GPSCorrectionSourceDiagnostic> sourceDiagnostics() const { return _router.sourceDiagnostics(); }

    QList<GPSCorrectionDestinationDiagnostic> destinationDiagnostics() const
    {
        return _router.destinationDiagnostics();
    }

    QList<GPSCorrectionStreamDiagnostic> sourceInstances() const { return _router.sourceInstanceDiagnostics(); }

    /// Whether a fresh stream is selected for vehicles.
    bool hasSelectedStream() const;

    /// The fresh stream selected for vehicles, or a stream with an Unknown source when none is.
    GPSCorrectionStreamDiagnostic selectedStream() const;

    /// Received rate of the selected stream's source category.
    quint64 selectedBytesPerSecond() const { return _selectedBytesPerSecond; }

    Q_INVOKABLE static QString sourceName(int source);

signals:
    void sourceInstancesChanged();
    void selectedBytesPerSecondChanged();

private:
    void _applyRoutingSettings();
    void _applyUdpInputSettings();
    void _applyUdpOutputSettings();

    void _scheduleSourcesChanged();
    void _refreshDiagnostics();

    GPSCorrectionRouter _router;
    GPSCorrectionEventModel _eventModel;
    SourceModel _sourceModel{this};
    DestinationModel _destinationModel{this};
    QTimer _diagnosticsTimer;
    QTimer _healthTimer;
    RTCMMavlink _rtcmMavlink;
    RTCMUdpInput _udpInput;
    GPSCorrectionSourceRegistration _udpRegistration;
    UdpForwarder _udpOutput{this};
    QPointer<GPSCorrectionSettings> _settings;
    // Last published instances; notifications fire only when the list changes.
    QList<GPSCorrectionStreamDiagnostic> _sourceInstances;
    quint64 _selectedBytesPerSecond = 0;
    GPSRevision _udpConfigurationRevision;
    int _ingressDepth = 0;
    bool _finalDiagnosticsPending = false;
    bool _shutdown = false;
    GPSNotificationQueue _notifications{this};
};
