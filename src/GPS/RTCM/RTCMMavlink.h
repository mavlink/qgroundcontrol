#pragma once

#include <functional>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

#include "DataRateTracker.h"
#include "RTCMMavlinkPacket.h"

class RTCMMavlink : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(quint64 totalBytesSent READ totalBytesSent NOTIFY bandwidthChanged)
    Q_PROPERTY(double bandwidthKBps READ bandwidthKBps NOTIFY bandwidthChanged)
    Q_PROPERTY(quint64 totalBytesSubmitted READ totalBytesSubmitted NOTIFY deliveryStatsChanged)

public:
    struct Output
    {
        QString id;
        quint64 session = 0;
        /// Admission only, not physical delivery or receiver acknowledgement.
        std::function<bool(const GpsRtcmPacket&)> submit;
    };

    struct Admission
    {
        QString id;
        quint64 session = 0;
        quint64 queuedBytes = 0;
        /// Includes the zero-length terminator when fragmentation requires one.
        bool complete = false;
    };

    using OutputProvider = std::function<QList<Output>()>;

    explicit RTCMMavlink(QObject* parent = nullptr);
    ~RTCMMavlink() override;

    quint64 totalBytesSent() const { return _rateTracker.totalBytes(); }

    double bandwidthKBps() const { return _rateTracker.kBps(); }

    quint64 totalBytesSubmitted() const { return _submittedBytes; }

    void setOutputProvider(OutputProvider provider);
    QList<Admission> submitToOutputs(QByteArrayView data);
signals:
    void bandwidthChanged();
    void deliveryStatsChanged();

private:
    OutputProvider _outputProvider;
    quint64 _outputRevision = 0;
    quint64 _submittedBytes = 0;
    uint8_t _sequenceId = 0;
    DataRateTracker _rateTracker;
    bool _submitting = false;
};
