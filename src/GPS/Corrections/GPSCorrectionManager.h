#pragma once

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
    Q_PROPERTY(QString activeInstance READ activeInstance NOTIFY sourcesChanged)
    Q_PROPERTY(GPSCorrectionEventModel* events READ events CONSTANT)
    Q_PROPERTY(QVariantList destinations READ destinations NOTIFY sourcesChanged)

    friend class GPSCorrectionManagerTest;

public:
    enum class RoutingPolicy
    {
        Automatic,
        Manual,
        All
    };
    Q_ENUM(RoutingPolicy)

    struct RoutingConfiguration
    {
        RoutingPolicy policy = RoutingPolicy::Automatic;
        GPSCorrectionSource source = GPSCorrectionSource::Unknown;
        QString instance;
    };

    explicit GPSCorrectionManager(QObject* parent = nullptr);
    ~GPSCorrectionManager() override;

    void init(GPSCorrectionSettings* settings);
    void shutdown();
    void configureNtripUdpOutput(bool enabled, const QString& address, quint16 port);

    RTCMMavlink* rtcmMavlink() { return &_rtcmMavlink; }

    void applyRoutingConfiguration(const RoutingConfiguration& configuration);
    GPSCorrectionSourceRegistration registerSource(GPSCorrectionSource source, const QString& instance = {});
    void acceptIngress(const GPSCorrectionIngress& ingress);
    void setSelectedSource(GPSCorrectionSource source);

    GPSCorrectionSource selectedSource() const { return _router.selectedSource(); }

    void setRoutingPolicy(RoutingPolicy policy);
    RoutingPolicy routingPolicy() const;
    void setSelectedInstance(const QString& instance);

    QString activeInstance() const { return _router.activeInstance(); }

    void addSink(const QString& id, GPSCorrectionRouter::Sink sink);
    void removeSink(const QString& id);
    void addDetailedSink(const QString& id, GPSCorrectionRouter::DetailedSink sink, bool reportsWrites = true);
    void recordDeliveries(const QList<GPSCorrectionDelivery>& deliveries);
    void invalidateDestination(const QString& id, quint64 destinationSession);

    GPSCorrectionEventModel* events() { return &_eventModel; }

    QVariantList destinations() const;

    QVariantList sources() const;
    QVariantList sourceInstances() const;

signals:
    void sourcesChanged();
    void sourceInstancesChanged();
    void correctionRouted(const GPSCorrectionFrame& frame);
    void selectedSourceChanged();

private:
    void _applyUdpInputSettings();

    void _scheduleSourcesChanged();
    void _refreshDiagnostics();
    void _refreshSourceInstances();

    GPSCorrectionRouter _router;
    GPSCorrectionEventModel _eventModel;
    QTimer _diagnosticsTimer;
    QTimer _healthTimer;
    RTCMMavlink _rtcmMavlink;
    RTCMUdpInput _udpInput;
    GPSCorrectionSourceRegistration _udpRegistration;
    UdpForwarder _ntripUdpOutput{this};
    QPointer<GPSCorrectionSettings> _settings;
    QVariantList _lastSourceInstances;
    quint64 _udpConfigurationRevision = 0;
    bool _shutdown = false;
};
