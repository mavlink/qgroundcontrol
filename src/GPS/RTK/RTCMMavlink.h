#pragma once

#include <functional>

#include "DataRateTracker.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtQmlIntegration/QtQmlIntegration>

typedef struct __mavlink_gps_rtcm_data_t mavlink_gps_rtcm_data_t;

Q_DECLARE_LOGGING_CATEGORY(RTCMMavlinkLog)

class GPSRtk;

class RTCMMavlink : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double bandwidthKBps READ bandwidthKBps NOTIFY bandwidthChanged)
    Q_PROPERTY(quint64 totalBytesSent READ totalBytesSent NOTIFY totalBytesSentChanged)

public:
    using MessageSender = std::function<void(const mavlink_gps_rtcm_data_t &)>;

    explicit RTCMMavlink(QObject *parent = nullptr);
    ~RTCMMavlink() override;

    void setMessageSender(MessageSender sender) { _sender = std::move(sender); }

    /// Set a sender with a context object — the sender is skipped if context is destroyed.
    void setMessageSender(QObject *context, MessageSender sender);

    double bandwidthKBps() const { return _rateTracker.kBps(); }
    quint64 totalBytesSent() const { return _rateTracker.totalBytes(); }

    void addGpsDevice(GPSRtk *device);
    void removeGpsDevice(GPSRtk *device);

public slots:
    void RTCMDataUpdate(QByteArrayView data);
    void routeToBaseStation(QByteArrayView data);

signals:
    void bandwidthChanged();
    void totalBytesSentChanged();

private:
    static void _defaultSendToVehicles(const mavlink_gps_rtcm_data_t &data);

    QPointer<QObject> _senderContext;
    bool _useSenderContext = false;
    MessageSender _sender;
    uint8_t _sequenceId = 0;
    DataRateTracker _rateTracker;
    QTimer _bandwidthDecayTimer;
    QList<QPointer<GPSRtk>> _gpsDevices;
};
