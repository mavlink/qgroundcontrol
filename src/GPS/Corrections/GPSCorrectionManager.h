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

class GPSCorrectionSettings;

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

    explicit GPSCorrectionManager(QObject* parent = nullptr);
    ~GPSCorrectionManager() override;

    void init(GPSCorrectionSettings* settings);
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
    void addDetailedSink(const QString& id, GPSCorrectionRouter::DetailedSink sink, bool reportsWrites = true);
    void recordDeliveries(const QList<GPSCorrectionDelivery>& deliveries);
    void invalidateDestination(const QString& id, quint64 destinationSession);
    void recordRejectedFrame(const GPSCorrectionFrame& frame, GPSCorrectionReason reason);

    GPSCorrectionEventModel* events() { return &_eventModel; }

    QVariantList destinations() const;

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
    void _refreshDiagnostics();

    GPSCorrectionRouter _router;
    GPSCorrectionEventModel _eventModel;
    QTimer _diagnosticsTimer;
    QTimer _healthTimer;
    RTCMMavlink _rtcmMavlink;
    RTCMUdpInput _udpInput;
    QPointer<GPSCorrectionSettings> _settings;
    bool _shutdown = false;
};
