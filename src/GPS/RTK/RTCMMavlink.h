#pragma once

#include <functional>

#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>

typedef struct __mavlink_gps_rtcm_data_t mavlink_gps_rtcm_data_t;

Q_DECLARE_LOGGING_CATEGORY(RTCMMavlinkLog)

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

    double bandwidthKBps() const { return _bandwidthKBps; }
    quint64 totalBytesSent() const { return _totalBytesSent; }

public slots:
    void RTCMDataUpdate(QByteArrayView data);

signals:
    void bandwidthChanged();
    void totalBytesSentChanged();

private:
    void _calculateBandwidth(qsizetype bytes);
    static void _defaultSendToVehicles(const mavlink_gps_rtcm_data_t &data);

    MessageSender _sender;
    uint8_t _sequenceId = 0;
    qsizetype _bandwidthByteCounter = 0;
    QElapsedTimer _bandwidthTimer;
    double _bandwidthKBps = 0.0;
    quint64 _totalBytesSent = 0;
};
